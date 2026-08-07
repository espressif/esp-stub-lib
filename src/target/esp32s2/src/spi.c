/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * SPI (download boot) transport for the ESP32-S2 flasher stub.
 *
 * GP-SPI2 slave half-duplex + integrated DMA are already set up by the ROM SPI
 * download boot. The driver reuses that state and the ROM DMA loaders, and only
 * drives the shared W0..W3 handshake registers (VER/RXSTA/TXSTA/CMD).
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/err.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/spi.h>

#include <private/helpers.h>
#include <private/spi_slave_protocol.h>

#include <soc/reg_base.h>
#include <soc/spi_reg.h>

#define SPI_SLV_HW        ((void *)DR_REG_SPI2_BASE) /* peripheral base as ROM handle */

#define SPI_SLV_REG_VER   SPI_W0_REG
#define SPI_SLV_REG_RXSTA SPI_W1_REG
#define SPI_SLV_REG_TXSTA SPI_W2_REG
#define SPI_SLV_REG_CMD   SPI_W3_REG

#define SPI_SLV_RX_DONE   SPI_IN_SUC_EOF_INT_RAW    /* WRDMA frame received   */
#define SPI_SLV_TX_DONE   SPI_OUT_TOTAL_EOF_INT_RAW /* RDDMA frame copied out */

extern void spi_slave_rom_rxdma_load(void *hw, uint8_t *buf, uint32_t len);
extern void spi_slave_rom_txdma_load(void *hw, const uint8_t *buf, uint32_t len);

extern void esp_rom_software_reset_cpu(uint32_t);

static uint32_t s_seq_rx;
static uint32_t s_seq_tx;
static volatile bool s_rx_armed;

bool stub_target_spi_is_active(void)
{
    /* Active only if the ROM left GP-SPI2 in slave mode and CMD holds a
     * download handshake value (READY = host connected, DONE = stub loaded). */
    if (!(READ_PERI_REG(SPI_SLAVE_REG) & SPI_SLAVE_MODE)) {
        return false;
    }
    uint32_t cmd = READ_PERI_REG(SPI_SLV_REG_CMD) & 0xFFU;
    return (cmd == SPI_SLV_CMD_READY) || (cmd == SPI_SLV_CMD_DONE);
}

void stub_target_spi_init(void)
{
    /* ROM download boot leaves CMD = DONE; the host's stub-connect handshake
     * busy-waits for IDLE, so this reset is required. */
    WRITE_PERI_REG(SPI_SLV_REG_CMD, SPI_SLV_CMD_IDLE);

    s_seq_rx = SPI_SLV_STATE_INIT;
    s_seq_tx = SPI_SLV_STATE_INIT;
    s_rx_armed = false;
}

int stub_target_spi_rearm(uint8_t *buf, size_t max_size)
{
    if (s_rx_armed) {
        return STUB_LIB_OK;
    }
    if (buf == NULL || max_size == 0) {
        return STUB_LIB_ERR_INVALID_ARG;
    }

    WRITE_PERI_REG(SPI_DMA_INT_CLR_REG, SPI_SLV_RX_DONE);
    spi_slave_rom_rxdma_load(SPI_SLV_HW, buf, (uint32_t)max_size);

    s_seq_rx ^= SPI_SLV_STA_TOGGLE;
    uint32_t rxsta = s_seq_rx | ((uint32_t)max_size << SPI_SLV_STA_LEN_SHIFT);
    s_seq_rx &= ~SPI_SLV_STA_INIT;
    WRITE_PERI_REG(SPI_SLV_REG_RXSTA, rxsta);

    s_rx_armed = true;
    return STUB_LIB_OK;
}

/* Poll the shared CMD handshake (no ISR). */
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

int stub_target_spi_tx_frame(const void *data, size_t len)
{
    WRITE_PERI_REG(SPI_DMA_INT_CLR_REG, SPI_SLV_TX_DONE);
    spi_slave_rom_txdma_load(SPI_SLV_HW, (const uint8_t *)data, (uint32_t)len);

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
    return STUB_LIB_OK;
}
