/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * SPI (download boot) transport for the ESP32-P4 flasher stub.
 *
 * GP-SPI2 slave + AXI-DMA (channel 0) are left configured by the ROM download
 * boot; the driver only arms the DMA and drives the shared W0..W3 handshake
 * registers.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/err.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>
#include <target/spi.h>

#include <private/helpers.h>
#include <private/spi_slave_protocol.h>

#include <soc/axi_dma_reg.h>
#include <soc/interrupt_core0_reg.h>
#include <soc/spi_reg.h>

#define SPI_SLV_HOST        2 /* GP-SPI2 */

#define SPI_SLV_REG_VER     SPI_W0_REG(SPI_SLV_HOST)
#define SPI_SLV_REG_RXSTA   SPI_W1_REG(SPI_SLV_HOST)
#define SPI_SLV_REG_TXSTA   SPI_W2_REG(SPI_SLV_HOST)
#define SPI_SLV_REG_CMD     SPI_W3_REG(SPI_SLV_HOST)

/* Host WRDMA writes to us (RX); host RDDMA reads from us (TX). */
#define SPI_SLV_RX_DONE     SPI_SLV_WR_DMA_DONE_INT_RAW
#define SPI_SLV_RX_DONE_CLR SPI_SLV_WR_DMA_DONE_INT_CLR
#define SPI_SLV_TX_DONE     SPI_SLV_RD_DMA_DONE_INT_RAW
#define SPI_SLV_TX_DONE_CLR SPI_SLV_RD_DMA_DONE_INT_CLR

/* DMA descriptor (ROM lldesc_spi_s layout); P4 needs it 8-byte aligned. */
typedef struct __attribute__((aligned(8))) lldesc_spi_s {
    volatile uint32_t size : 12, length : 12, offset : 5, sosf : 1, eof : 1, owner : 1;
    volatile uint8_t *buf;
    struct lldesc_spi_s *next;
    uint32_t pad_to_8byte;
} lldesc_spi_t;

#define LLDESC_SPI_MAX_BUFFER_SIZE (4096U - 4U)
#define LLDESC_SPI_SIZE_MASK       0xFFFU

/* Chain sized to cover FRAME_BUFFER_SIZE (0x4107). */
#define SPI_SLV_RX_DESC_COUNT      5U
#define SPI_SLV_RX_MAX_LEN         (SPI_SLV_RX_DESC_COUNT * LLDESC_SPI_MAX_BUFFER_SIZE)

static lldesc_spi_t dmadesc_tx;
static lldesc_spi_t dmadesc_rx[SPI_SLV_RX_DESC_COUNT];

extern void esp_rom_software_reset_cpu(uint32_t);

static uint32_t s_seq_rx;
static uint32_t s_seq_tx;
static volatile bool s_rx_armed;

/* Saved so tx_frame() can re-arm RX after a TX rewrites SPI_DMA_CONF. */
static uint8_t *s_rx_buf;
static uint32_t s_rx_len;

bool stub_target_spi_is_active(void)
{
    if (!(READ_PERI_REG(SPI_SLAVE_REG(SPI_SLV_HOST)) & SPI_SLAVE_MODE)) {
        return false;
    }
    uint32_t cmd = READ_PERI_REG(SPI_SLV_REG_CMD) & 0xFFU;
    return (cmd == SPI_SLV_CMD_READY) || (cmd == SPI_SLV_CMD_DONE);
}

void stub_target_spi_init(void)
{
    /* Unroute the ROM's SPI2 ISR so it can't race our polled handshake. */
    WRITE_PERI_REG(INTERRUPT_CORE0_SPI2_INT_MAP_REG, 0);

    /* ROM download boot leaves CMD = DONE; the host's stub-connect handshake
     * busy-waits for IDLE, so this reset is required. */
    WRITE_PERI_REG(SPI_SLV_REG_CMD, SPI_SLV_CMD_IDLE);

    s_seq_rx = SPI_SLV_STATE_INIT;
    s_seq_tx = SPI_SLV_STATE_INIT;
    s_rx_armed = false;
}

/* Split buf into <=4092-byte descriptors and (re)start the RX inlink. */
static void spi_slv_rxdma_arm(uint8_t *buf, uint32_t len)
{
    uint32_t remaining = len;
    uint8_t *buf_pos = buf;
    uint32_t desc_idx = 0;
    while (remaining) {
        uint32_t chunk = (remaining > LLDESC_SPI_MAX_BUFFER_SIZE) ? LLDESC_SPI_MAX_BUFFER_SIZE : remaining;
        uint32_t aligned_len = (chunk + 3U) & ~3U; /* 4-byte DMA granularity */
        dmadesc_rx[desc_idx].size = aligned_len & LLDESC_SPI_SIZE_MASK;
        dmadesc_rx[desc_idx].length = aligned_len & LLDESC_SPI_SIZE_MASK;
        dmadesc_rx[desc_idx].buf = buf_pos;
        dmadesc_rx[desc_idx].sosf = 0;
        dmadesc_rx[desc_idx].owner = 1;
        if (remaining <= chunk) {
            dmadesc_rx[desc_idx].eof = 1;
            dmadesc_rx[desc_idx].next = NULL;
        } else {
            dmadesc_rx[desc_idx].eof = 0;
            dmadesc_rx[desc_idx].next = &dmadesc_rx[desc_idx + 1];
        }
        remaining -= chunk;
        buf_pos += chunk;
        desc_idx++;
    }
    stub_target_cache_writeback_addr((uint32_t)(uintptr_t)dmadesc_rx, desc_idx * sizeof(dmadesc_rx[0]));

    WRITE_PERI_REG(SPI_DMA_INT_CLR_REG(SPI_SLV_HOST), SPI_SLV_RX_DONE_CLR);
    SET_PERI_REG_MASK(AXI_DMA_IN_CONF0_CH0_REG, AXI_DMA_IN_RST_CH0);
    CLEAR_PERI_REG_MASK(AXI_DMA_IN_CONF0_CH0_REG, AXI_DMA_IN_RST_CH0);
    WRITE_PERI_REG(SPI_DMA_CONF_REG(SPI_SLV_HOST), SPI_DMA_AFIFO_RST | SPI_BUF_AFIFO_RST | SPI_RX_AFIFO_RST);
    SET_PERI_REG_MASK(SPI_DMA_CONF_REG(SPI_SLV_HOST), SPI_DMA_RX_ENA);
    WRITE_PERI_REG(AXI_DMA_IN_LINK2_CH0_REG, (uint32_t)(uintptr_t)dmadesc_rx);
    /* Clear a stale dscr_err/underflow, else the inlink never runs. */
    WRITE_PERI_REG(AXI_DMA_IN_INT_CLR_CH0_REG,
                   AXI_DMA_IN_DSCR_ERR_CH0_INT_CLR | AXI_DMA_INFIFO_L1_UDF_CH0_INT_CLR |
                       AXI_DMA_INFIFO_L2_UDF_CH0_INT_CLR | AXI_DMA_INFIFO_L3_UDF_CH0_INT_CLR);
    SET_PERI_REG_MASK(AXI_DMA_IN_LINK1_CH0_REG, AXI_DMA_INLINK_START_CH0);
}

int stub_target_spi_rearm(uint8_t *buf, size_t max_size)
{
    if (s_rx_armed) {
        return STUB_LIB_OK;
    }
    if (buf == NULL || max_size == 0) {
        return STUB_LIB_ERR_INVALID_ARG;
    }

    if (max_size > SPI_SLV_RX_MAX_LEN) {
        max_size = SPI_SLV_RX_MAX_LEN;
    }

    spi_slv_rxdma_arm(buf, (uint32_t)max_size);
    s_rx_buf = buf;
    s_rx_len = (uint32_t)max_size;

    s_seq_rx ^= SPI_SLV_STA_TOGGLE;
    uint32_t rxsta = s_seq_rx | ((uint32_t)max_size << SPI_SLV_STA_LEN_SHIFT);
    s_seq_rx &= ~SPI_SLV_STA_INIT;
    WRITE_PERI_REG(SPI_SLV_REG_RXSTA, rxsta);

    s_rx_armed = true;
    return STUB_LIB_OK;
}

static void spi_slv_service_cmd(void)
{
    uint32_t cmd = READ_PERI_REG(SPI_SLV_REG_CMD) & 0xFFU;
    switch (cmd) {
    case SPI_SLV_CMD_READY:
        WRITE_PERI_REG(SPI_SLV_REG_CMD, SPI_SLV_CMD_READY);
        break;
    case SPI_SLV_CMD_REBOOT:
        esp_rom_software_reset_cpu(0);
        break;
    default:
        break;
    }
}

bool stub_target_spi_take_rx_frame(size_t *out_len)
{
    spi_slv_service_cmd();

    if (!(READ_PERI_REG(SPI_DMA_INT_RAW_REG(SPI_SLV_HOST)) & SPI_SLV_RX_DONE)) {
        return false;
    }
    WRITE_PERI_REG(SPI_DMA_INT_CLR_REG(SPI_SLV_HOST), SPI_SLV_RX_DONE_CLR);

    /* Stub runs from cached L2MEM; drop stale lines so we read the DMA'd bytes. */
    if (s_rx_buf != NULL) {
        stub_target_cache_invalidate_addr((uint32_t)(uintptr_t)s_rx_buf, s_rx_len);
    }

    if (out_len) {
        uint32_t bitlen = READ_PERI_REG(SPI_SLAVE1_REG(SPI_SLV_HOST)) & SPI_SLV_DATA_BITLEN;
        *out_len = bitlen / 8U;
    }

    s_rx_armed = false;
    return true;
}

int stub_target_spi_tx_frame(const void *data, size_t len)
{
    WRITE_PERI_REG(SPI_DMA_INT_CLR_REG(SPI_SLV_HOST), SPI_SLV_TX_DONE_CLR);

    dmadesc_tx.size = (uint32_t)len & LLDESC_SPI_SIZE_MASK;
    dmadesc_tx.length = (uint32_t)len & LLDESC_SPI_SIZE_MASK;
    dmadesc_tx.buf = (uint8_t *)data;
    dmadesc_tx.sosf = 0;
    dmadesc_tx.eof = 1;
    dmadesc_tx.owner = 1;
    dmadesc_tx.next = NULL;
    stub_target_cache_writeback_addr((uint32_t)(uintptr_t)data, (uint32_t)len);
    stub_target_cache_writeback_addr((uint32_t)(uintptr_t)&dmadesc_tx, sizeof(dmadesc_tx));

    SET_PERI_REG_MASK(AXI_DMA_OUT_CONF0_CH0_REG, AXI_DMA_OUT_RST_CH0);
    CLEAR_PERI_REG_MASK(AXI_DMA_OUT_CONF0_CH0_REG, AXI_DMA_OUT_RST_CH0);
    WRITE_PERI_REG(AXI_DMA_OUT_LINK2_CH0_REG, (uint32_t)(uintptr_t)&dmadesc_tx);
    SET_PERI_REG_MASK(AXI_DMA_OUT_LINK1_CH0_REG, AXI_DMA_OUTLINK_START_CH0);
    WRITE_PERI_REG(SPI_DMA_CONF_REG(SPI_SLV_HOST), SPI_DMA_AFIFO_RST | SPI_BUF_AFIFO_RST | SPI_RX_AFIFO_RST);
    SET_PERI_REG_MASK(SPI_DMA_CONF_REG(SPI_SLV_HOST), SPI_DMA_TX_ENA);

    s_seq_tx ^= SPI_SLV_STA_TOGGLE;
    uint32_t txsta = s_seq_tx | ((uint32_t)len << SPI_SLV_STA_LEN_SHIFT);
    s_seq_tx &= ~SPI_SLV_STA_INIT;
    WRITE_PERI_REG(SPI_SLV_REG_TXSTA, txsta);

    /* Leave TXSTA set until the next frame; clearing it early races the host. */
    uint64_t timeout_us = SPI_SLV_TX_TIMEOUT_US;
    int ret = stub_target_wait_reg_bit_set(SPI_DMA_INT_RAW_REG(SPI_SLV_HOST), SPI_SLV_TX_DONE, &timeout_us);
    if (ret != STUB_LIB_OK) {
        return STUB_LIB_FAIL;
    }
    WRITE_PERI_REG(SPI_DMA_INT_CLR_REG(SPI_SLV_HOST), SPI_SLV_TX_DONE_CLR);

    /* Enabling TX DMA rewrote SPI_DMA_CONF and disarmed RX DMA; re-arm it. */
    if (s_rx_armed && s_rx_buf != NULL) {
        spi_slv_rxdma_arm(s_rx_buf, s_rx_len);
    }
    return STUB_LIB_OK;
}
