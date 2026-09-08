/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * SPI (download boot) transport for the ESP32-S2 flasher stub.
 *
 * GP-SPI2 slave half-duplex + the SPI-integrated DMA are left configured by the
 * ROM SPI download boot. The driver reuses that state, builds its own linked
 * DMA descriptors and drives the SPI-integrated DMA (in/out link) registers
 * directly, and drives the shared W0..W3 handshake registers (VER/RXSTA/TXSTA/
 * CMD). Unlike the GDMA targets (e.g. ESP32-S3), the ESP32-S2 DMA is part of the
 * SPI peripheral, so the descriptors are mounted via SPI_DMA_IN_LINK_REG /
 * SPI_DMA_OUT_LINK_REG rather than a separate GDMA channel.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/err.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/slave_spi.h>

#include <private/helpers.h>
#include <private/spi_slave_protocol.h>

#include <soc/spi_reg.h>

#define SPI_SLV_REG_VER   SPI_W0_REG
#define SPI_SLV_REG_RXSTA SPI_W1_REG
#define SPI_SLV_REG_TXSTA SPI_W2_REG
#define SPI_SLV_REG_CMD   SPI_W3_REG

#define SPI_SLV_RX_DONE   SPI_IN_SUC_EOF_INT_RAW    /* WRDMA frame received   */
#define SPI_SLV_TX_DONE   SPI_OUT_TOTAL_EOF_INT_RAW /* RDDMA frame copied out */

/* DMA descriptor (ROM lldesc_spi_t layout, 3 words). */
typedef struct lldesc_spi_s {
    volatile uint32_t size : 12, length : 12, offset : 5, sosf : 1, eof : 1, owner : 1;
    volatile uint8_t *buf;
    struct lldesc_spi_s *next;
} lldesc_spi_t;

#define LLDESC_SPI_MAX_BUFFER_SIZE (4096U - 4U)
#define LLDESC_SPI_SIZE_MASK       0xFFFU

/* Chain sized to cover FRAME_BUFFER_SIZE (0x4107). */
#define SPI_SLV_RX_DESC_COUNT      5U
#define SPI_SLV_RX_MAX_LEN         (SPI_SLV_RX_DESC_COUNT * LLDESC_SPI_MAX_BUFFER_SIZE)

static lldesc_spi_t dmadesc_tx;
static lldesc_spi_t dmadesc_rx[SPI_SLV_RX_DESC_COUNT];

static uint32_t s_seq_rx;
static uint32_t s_seq_tx;
static volatile bool s_rx_armed;

/* Saved so tx_frame() can re-arm RX after a TX resets the shared DMA state. */
static uint8_t *s_rx_buf;
static uint32_t s_rx_len;

bool stub_target_slave_spi_is_active(void)
{
    /* Active only if the ROM left GP-SPI2 in slave mode and CMD holds a
     * download handshake value (READY = host connected, DONE = stub loaded). */
    if (!(READ_PERI_REG(SPI_SLAVE_REG) & SPI_SLAVE_MODE)) {
        return false;
    }
    uint32_t cmd = READ_PERI_REG(SPI_SLV_REG_CMD) & 0xFFU;
    return (cmd == SPI_SLV_CMD_READY) || (cmd == SPI_SLV_CMD_DONE);
}

void stub_target_slave_spi_init(void)
{
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
        /* Receive needs the DMA length rounded up to the next 32-bit boundary. */
        uint32_t dma_len = (chunk + 3U) & ~3U;
        dmadesc_rx[desc_idx].size = dma_len & LLDESC_SPI_SIZE_MASK;
        dmadesc_rx[desc_idx].length = dma_len & LLDESC_SPI_SIZE_MASK;
        dmadesc_rx[desc_idx].offset = 0;
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

    /* Reset the shared DMA AHB master FIFO and the in (RX) DMA engine. */
    SET_PERI_REG_MASK(SPI_DMA_CONF_REG, SPI_AHBM_RST | SPI_AHBM_FIFO_RST);
    CLEAR_PERI_REG_MASK(SPI_DMA_CONF_REG, SPI_AHBM_RST | SPI_AHBM_FIFO_RST);
    SET_PERI_REG_MASK(SPI_DMA_CONF_REG, SPI_IN_RST);
    CLEAR_PERI_REG_MASK(SPI_DMA_CONF_REG, SPI_IN_RST);
    WRITE_PERI_REG(SPI_DMA_INT_CLR_REG, SPI_SLV_RX_DONE);
    /* Mount the inlink descriptor, enable the RX DMA path and start it. */
    WRITE_PERI_REG(SPI_DMA_IN_LINK_REG, ((uint32_t)(uintptr_t)dmadesc_rx & SPI_INLINK_ADDR) | SPI_DMA_RX_ENA);
    SET_PERI_REG_MASK(SPI_DMA_IN_LINK_REG, SPI_INLINK_START);
}

int stub_target_slave_spi_rearm(uint8_t *buf, size_t max_size)
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

bool stub_target_slave_spi_take_rx_frame(size_t *out_len)
{
    if ((READ_PERI_REG(SPI_SLV_REG_CMD) & 0xFFU) == SPI_SLV_CMD_READY) {
        WRITE_PERI_REG(SPI_SLV_REG_CMD, SPI_SLV_CMD_READY);
    }

    if (!(READ_PERI_REG(SPI_DMA_INT_RAW_REG) & SPI_SLV_RX_DONE)) {
        return false;
    }
    WRITE_PERI_REG(SPI_DMA_INT_CLR_REG, SPI_SLV_RX_DONE);

    if (out_len) {
        /* S2 reports the received length directly in bytes. */
        *out_len = READ_PERI_REG(SPI_SLV_RD_BYTE_REG) & SPI_SLV_DATA_BYTELEN;
    }

    s_rx_armed = false;
    return true;
}

int stub_target_slave_spi_tx_frame(const void *data, size_t len)
{
    /* TX uses a single descriptor whose size field is 12 bits; reject frames
     * that would overflow it (a len of 4096 would otherwise mask to 0). */
    if (len > LLDESC_SPI_MAX_BUFFER_SIZE) {
        return STUB_LIB_ERR_INVALID_ARG;
    }

    WRITE_PERI_REG(SPI_DMA_INT_CLR_REG, SPI_SLV_TX_DONE);

    dmadesc_tx.size = (uint32_t)len & LLDESC_SPI_SIZE_MASK;
    dmadesc_tx.length = (uint32_t)len & LLDESC_SPI_SIZE_MASK;
    dmadesc_tx.buf = (uint8_t *)data;
    dmadesc_tx.sosf = 0;
    dmadesc_tx.eof = 1;
    dmadesc_tx.owner = 1;
    dmadesc_tx.next = NULL;

    /* Reset the shared DMA AHB master FIFO and the out (TX) DMA engine. */
    SET_PERI_REG_MASK(SPI_DMA_CONF_REG, SPI_AHBM_RST | SPI_AHBM_FIFO_RST);
    CLEAR_PERI_REG_MASK(SPI_DMA_CONF_REG, SPI_AHBM_RST | SPI_AHBM_FIFO_RST);
    SET_PERI_REG_MASK(SPI_DMA_CONF_REG, SPI_OUT_RST);
    CLEAR_PERI_REG_MASK(SPI_DMA_CONF_REG, SPI_OUT_RST);
    /* Mount the outlink descriptor, enable the TX DMA path and start it. */
    WRITE_PERI_REG(SPI_DMA_OUT_LINK_REG, ((uint32_t)(uintptr_t)&dmadesc_tx & SPI_OUTLINK_ADDR) | SPI_DMA_TX_ENA);
    SET_PERI_REG_MASK(SPI_DMA_OUT_LINK_REG, SPI_OUTLINK_START);

    s_seq_tx ^= SPI_SLV_STA_TOGGLE;
    uint32_t txsta = s_seq_tx | ((uint32_t)len << SPI_SLV_STA_LEN_SHIFT);
    s_seq_tx &= ~SPI_SLV_STA_INIT;
    WRITE_PERI_REG(SPI_SLV_REG_TXSTA, txsta);

    /* Leave the TXSTA length until the next frame: the host keys off the toggle
     * bit, and clearing it early races its poll for short frames (e.g. MD5). */
    uint64_t timeout_us = SPI_SLV_TX_TIMEOUT_US;
    int ret = stub_target_wait_reg_bit_set(SPI_DMA_INT_RAW_REG, SPI_SLV_TX_DONE, &timeout_us);
    if (ret != STUB_LIB_OK) {
        return STUB_LIB_FAIL;
    }
    WRITE_PERI_REG(SPI_DMA_INT_CLR_REG, SPI_SLV_TX_DONE);

    /* Resetting the shared DMA AHB master above disarmed the in-flight RX DMA;
     * re-arm it so the next host frame is received. */
    if (s_rx_armed && s_rx_buf != NULL) {
        spi_slv_rxdma_arm(s_rx_buf, s_rx_len);
    }
    return STUB_LIB_OK;
}
