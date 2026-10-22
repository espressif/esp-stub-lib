/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * SPI NAND transaction on GPSPI2, shared by the chips whose GPSPI master has the same register layout.
 * The chip's nand.c enables the peripheral and routes the pins.
 */

#include <stddef.h>
#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/err.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/nand.h>

#include <private/helpers.h>

#include <soc/spi_reg.h>

#define NAND_SPI_NUM        2
#define NAND_SPI_TIMEOUT_US 100000U
#define NAND_SPI_DATA_WORDS 16

static int nand_spi_wait_clear(uint32_t cmd_bit)
{
    uint64_t timeout_us = NAND_SPI_TIMEOUT_US;
    if (stub_target_wait_reg_bit_clear(SPI_CMD_REG(NAND_SPI_NUM), cmd_bit, &timeout_us) != STUB_LIB_OK) {
        return NAND_ERR_SPI_FAIL;
    }
    return 0;
}

int stub_target_nand_bus_transaction(uint8_t cmd,
                                     uint32_t addr,
                                     uint8_t addr_bits,
                                     const uint8_t *tx_data,
                                     uint16_t tx_bits,
                                     uint8_t *rx_data,
                                     uint16_t rx_bits)
{
    int ret = nand_spi_wait_clear(SPI_USR);
    if (ret != 0) {
        return ret;
    }

    // Reset FIFOs
    REG_SET_BIT(SPI_DMA_CONF_REG(NAND_SPI_NUM), SPI_BUF_AFIFO_RST);
    REG_CLR_BIT(SPI_DMA_CONF_REG(NAND_SPI_NUM), SPI_BUF_AFIFO_RST);
    REG_SET_BIT(SPI_DMA_CONF_REG(NAND_SPI_NUM), SPI_RX_AFIFO_RST);
    REG_CLR_BIT(SPI_DMA_CONF_REG(NAND_SPI_NUM), SPI_RX_AFIFO_RST);

    // Clear W-registers to prevent stale data leaking into short RX transactions
    for (uint32_t i = 0; i < NAND_SPI_DATA_WORDS; i++) {
        REG_WRITE(SPI_W0_REG(NAND_SPI_NUM) + (i * sizeof(uint32_t)), 0);
    }

    // Build USER register.
    // Use full-duplex (DOUTDIN + MOSI) only when there is TX data to send.
    // For RX-only transactions (READ_FROM_CACHE), suppress MOSI and DOUTDIN so
    // the DI line is not driven with 0x00 bytes during the data phase.  On the
    // W25N01GV the DI line is sampled during READ_FROM_CACHE; driving 0x00 can
    // be misinterpreted as a PROGRAM_LOAD command (0x02 prefix) or corrupt the
    // chip's internal output-enable state, causing the cache to appear as zeros
    // for subsequent odd-block reads in a sequential scan.
    uint32_t user_val = SPI_CS_SETUP | SPI_CS_HOLD | SPI_USR_COMMAND;

    if (addr_bits > 0) {
        user_val |= SPI_USR_ADDR;
    }
    if (tx_bits > 0) {
        user_val |= SPI_USR_MOSI | SPI_DOUTDIN;
    }
    if (rx_bits > 0) {
        user_val |= SPI_USR_MISO;
    }

    REG_WRITE(SPI_USER_REG(NAND_SPI_NUM), user_val);

    // 8-bit command
    REG_WRITE(SPI_USER2_REG(NAND_SPI_NUM), (uint32_t)cmd);
    REG_SET_FIELD(SPI_USER2_REG(NAND_SPI_NUM), SPI_USR_COMMAND_BITLEN, 8U - 1U);

    // Address bit length and initial address value (written before SPI_UPDATE
    // so the CONF sync captures them for USR_ADDR_BITLEN).
    if (addr_bits > 0) {
        REG_WRITE(SPI_ADDR_REG(NAND_SPI_NUM), addr << (32 - addr_bits));
        REG_SET_FIELD(SPI_USER1_REG(NAND_SPI_NUM), SPI_USR_ADDR_BITLEN, addr_bits - 1);
    }

    // Data phase (TX and RX share the same clock cycles in full-duplex).
    // Always write W-registers (with TX data or zeros) so MOSI outputs 0x00
    // for RX-only transactions and the RX buffer is pre-cleared.
    uint16_t data_bits = MAX(tx_bits, rx_bits);

    if (data_bits > 0) {
        uint32_t data_bytes = ALIGN_UP((uint32_t)data_bits, 8U) / 8U;
        uint32_t tx_bytes = ALIGN_UP((uint32_t)tx_bits, 8U) / 8U;
        for (uint32_t i = 0; i < ALIGN_UP(data_bytes, 4U) / 4U; i++) {
            uint32_t word = 0;
            if (tx_data != NULL) {
                for (uint32_t j = 0; j < 4 && (i * 4 + j) < tx_bytes; j++) {
                    word |= ((uint32_t)tx_data[i * 4 + j]) << (j * 8);
                }
            }
            REG_WRITE(SPI_W0_REG(NAND_SPI_NUM) + (i * sizeof(uint32_t)), word);
        }
        REG_WRITE(SPI_MS_DLEN_REG(NAND_SPI_NUM), (uint32_t)(data_bits - 1));
    }

    // Synchronise APB register writes to the SPI clock domain.
    REG_WRITE(SPI_CMD_REG(NAND_SPI_NUM), SPI_UPDATE);
    ret = nand_spi_wait_clear(SPI_UPDATE);
    if (ret != 0) {
        return ret;
    }

    REG_WRITE(SPI_CMD_REG(NAND_SPI_NUM), SPI_USR);
    ret = nand_spi_wait_clear(SPI_USR);
    if (ret != 0) {
        return ret;
    }

    if (rx_data != NULL) {
        uint32_t rx_bytes = ALIGN_UP((uint32_t)rx_bits, 8U) / 8U;
        for (uint32_t i = 0; i < ALIGN_UP(rx_bytes, 4U) / 4U; i++) {
            uint32_t word = REG_READ(SPI_W0_REG(NAND_SPI_NUM) + (i * sizeof(uint32_t)));
            for (uint32_t j = 0; j < 4 && (i * 4 + j) < rx_bytes; j++) {
                rx_data[i * 4 + j] = (uint8_t)(word >> (j * 8));
            }
        }
    }

    return 0;
}
