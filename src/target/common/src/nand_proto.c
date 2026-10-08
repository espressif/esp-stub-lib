/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * SPI NAND command sequence shared by every chip that has a GPSPI master.
 * The chip file supplies nand_spi_init(); this file programs, reads, and erases.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/err.h>
#include <esp-stub-lib/log.h>
#include <esp-stub-lib/rom_wrappers.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/nand.h>
#include <target/nand_bus.h>

#include <soc/spi_reg.h>

#define NAND_SPI_NUM           2

#define SPI_BUSY_TIMEOUT_ITERS 100000

static int spi_nand_transaction(uint8_t cmd,
                                uint32_t addr,
                                uint8_t addr_bits,
                                const uint8_t *tx_data,
                                uint16_t tx_bits,
                                uint8_t *rx_data,
                                uint16_t rx_bits)
{
    int spi_timeout = SPI_BUSY_TIMEOUT_ITERS;
    while (REG_READ(SPI_CMD_REG(NAND_SPI_NUM)) & SPI_USR) {
        if (--spi_timeout <= 0) {
            return NAND_ERR_SPI_FAIL;
        }
        stub_lib_delay_us(1);
    }

    // Reset FIFOs
    REG_SET_BIT(SPI_DMA_CONF_REG(NAND_SPI_NUM), SPI_BUF_AFIFO_RST);
    REG_CLR_BIT(SPI_DMA_CONF_REG(NAND_SPI_NUM), SPI_BUF_AFIFO_RST);
    REG_SET_BIT(SPI_DMA_CONF_REG(NAND_SPI_NUM), SPI_RX_AFIFO_RST);
    REG_CLR_BIT(SPI_DMA_CONF_REG(NAND_SPI_NUM), SPI_RX_AFIFO_RST);

    // Clear W-registers to prevent stale data leaking into short RX transactions
    for (int _wr = 0; _wr < 16; _wr++) {
        REG_WRITE(SPI_W0_REG(NAND_SPI_NUM) + (_wr * 4), 0);
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
        uint32_t data_bytes = (uint32_t)((data_bits + 7) / 8);
        for (uint32_t i = 0; i < (data_bytes + 3) / 4; i++) {
            uint32_t word = 0;
            if (tx_bits > 0 && tx_data != NULL) {
                uint32_t tx_bytes = (uint32_t)((tx_bits + 7) / 8);
                for (uint32_t j = 0; j < 4 && (i * 4 + j) < tx_bytes; j++) {
                    word |= ((uint32_t)tx_data[i * 4 + j]) << (j * 8);
                }
            }
            REG_WRITE(SPI_W0_REG(NAND_SPI_NUM) + (i * 4), word);
        }
        REG_WRITE(SPI_MS_DLEN_REG(NAND_SPI_NUM), (uint32_t)(data_bits - 1));
    }

    // Synchronise APB register writes to the SPI clock domain.
    REG_WRITE(SPI_CMD_REG(NAND_SPI_NUM), SPI_UPDATE);
    spi_timeout = SPI_BUSY_TIMEOUT_ITERS;
    while (REG_READ(SPI_CMD_REG(NAND_SPI_NUM)) & SPI_UPDATE) {
        if (--spi_timeout <= 0) {
            return NAND_ERR_SPI_FAIL;
        }
        stub_lib_delay_us(1);
    }

    REG_WRITE(SPI_CMD_REG(NAND_SPI_NUM), SPI_USR);
    spi_timeout = SPI_BUSY_TIMEOUT_ITERS;
    while (REG_READ(SPI_CMD_REG(NAND_SPI_NUM)) & SPI_USR) {
        if (--spi_timeout <= 0) {
            return NAND_ERR_SPI_FAIL;
        }
        stub_lib_delay_us(1);
    }

    // Read RX data
    if (rx_bits > 0 && rx_data != NULL) {
        uint32_t rx_bytes = (uint32_t)((rx_bits + 7) / 8);
        for (uint32_t i = 0; i < (rx_bytes + 3) / 4; i++) {
            uint32_t word = REG_READ(SPI_W0_REG(NAND_SPI_NUM) + (i * 4));
            for (uint32_t j = 0; j < 4 && (i * 4 + j) < rx_bytes; j++) {
                rx_data[i * 4 + j] = (word >> (j * 8)) & 0xFF;
            }
        }
    }

    return 0;
}

// ---- NAND protocol layer --------------------------------------------------

// Zero-initialized so the plugin can live in BSS (no .data section needed).
// All fields are set explicitly in nand_attach() before use.
static nand_config_t s_nand_config;

/*
 * Two helpers for waiting on NAND operations, split by operation class:
 *
 * nand_wait_ready() — busy-only poll. Used after PAGE_READ and RESET.
 *   Populates s_last_status_byte so nand_check_ecc_after_read() can inspect
 *   the ECC field from the same status read.  Does NOT inspect P_FAIL/E_FAIL
 *   because those bits are sticky on the W25N01GV: they are set by a failed
 *   PROGRAM_EXECUTE or ERASE_BLOCK and cleared only by the next PROGRAM or
 *   ERASE operation (or by RESET).  A PAGE_READ following a failed program
 *   would therefore see a stale P_FAIL=1 and incorrectly return an error.
 *
 * nand_wait_program_erase() — busy poll + P_FAIL/E_FAIL check. Used after
 *   PROGRAM_EXECUTE and ERASE_BLOCK, where the fail bits are fresh and
 *   meaningful.  Delegates the busy loop to nand_wait_ready() so the timeout
 *   and delay logic stays in one place, then inspects s_last_status_byte for
 *   the fail bits.
 *
 * Timeout derived from NAND_ERASE_TIMEOUT_US with 10us poll interval.
 * Worst-case timing per W25N01GV (Winbond, JEDEC ID EF:AA21) datasheet:
 *   block erase tBERS max = 10ms, page program tPP max = 3ms,
 *   page read tRD max = 60us.
 */
static uint8_t s_last_status_byte;

static int nand_wait_ready(void)
{
    int timeout = (int)(NAND_ERASE_TIMEOUT_US / 10U);

    /* Initial delay: give the chip time to assert OIP (BUSY) before the first poll.
     * The W25N01GV sets OIP within ~1µs of CS deassertion but may not be visible on
     * the first APB/SPI read without a brief pause.  10µs is defensive. */
    stub_lib_delay_us(10);

    while (timeout-- > 0) {
        uint8_t status;

        int ret = spi_nand_transaction(NAND_CMD_READ_REGISTER, (uint32_t)NAND_REG_STATUS, 8, NULL, 0, &status, 8);
        if (ret != 0) {
            return ret;
        }

        s_last_status_byte = status;

        if ((status & NAND_STAT_BUSY) == 0) {
            return 0;
        }

        stub_lib_delay_us(10);
    }

    return NAND_ERR_TIMEOUT;
}

static int nand_wait_program_erase(void)
{
    int ret = nand_wait_ready();
    if (ret != 0) {
        return ret;
    }

    if (s_last_status_byte & NAND_STAT_ERASE_FAILED) {
        return NAND_ERR_ERASE_FAILED;
    }
    if (s_last_status_byte & NAND_STAT_PROGRAM_FAILED) {
        return NAND_ERR_PROGRAM_FAILED;
    }
    return 0;
}

/**
 * @brief Read NAND register (public API)
 */
int stub_target_nand_read_register(uint8_t reg, uint8_t *val)
{
    return spi_nand_transaction(NAND_CMD_READ_REGISTER, (uint32_t)reg, 8, NULL, 0, val, 8);
}

/* Internal alias kept for callers within this file */
static inline int nand_read_register(uint8_t reg, uint8_t *val)
{
    return stub_target_nand_read_register(reg, val);
}

/**
 * @brief Write NAND register
 */
static int nand_write_register(uint8_t reg, uint8_t val)
{
    uint8_t data[2] = { reg, val };
    return spi_nand_transaction(NAND_CMD_SET_REGISTER, 0, 0, data, 16, NULL, 0);
}

/**
 * @brief Issue write enable command
 */
static int nand_write_enable(void)
{
    return spi_nand_transaction(NAND_CMD_WRITE_ENABLE, 0, 0, NULL, 0, NULL, 0);
}

int stub_target_nand_read_id(uint8_t *manufacturer_id, uint16_t *device_id)
{
    uint8_t id_buf[3] = { 0 };
    int ret = spi_nand_transaction(NAND_CMD_READ_ID, 0x00, 8, NULL, 0, id_buf, 24);
    if (ret != 0) {
        return ret;
    }
    if (manufacturer_id) {
        *manufacturer_id = id_buf[0];
    }
    if (device_id) {
        *device_id = (uint16_t)(id_buf[1] << 8 | id_buf[2]);
    }
    return 0;
}

int stub_target_nand_attach(uint32_t hspi_arg, uint32_t page_size, uint32_t block_size)
{
    /* The column address still fits in 16 bits, with CA[15] selecting the even/odd cache. */
    if (page_size != 2048 && page_size != 4096) {
        STUB_LOGE("NAND page size %u is not 2048 or 4096\n", page_size);
        return STUB_LIB_ERR_INVALID_ARG;
    }
    if (block_size == 0 || (block_size % page_size) != 0) {
        STUB_LOGE("NAND block size %u is not a multiple of page size %u\n", block_size, page_size);
        return STUB_LIB_ERR_INVALID_ARG;
    }

    s_nand_config.page_size = page_size;
    s_nand_config.pages_per_block = block_size / page_size;
    s_nand_config.block_size = block_size;
    s_nand_config.initialized = false;
    s_last_status_byte = 0xFF;

    int ret = nand_spi_init(hspi_arg);
    if (ret != 0) {
        return ret;
    }

    /* Allow SPI2 peripheral to stabilize after clock enable and reset release */
    stub_lib_delay_us(5000);

    /* Device reset; W25N01GV datasheet tRST max = 500us for power-on reset */
    ret = spi_nand_transaction(NAND_CMD_RESET, 0, 0, NULL, 0, NULL, 0);
    if (ret != 0) {
        return NAND_ERR_RESET_FAILED;
    }

    /* Wait 10ms after reset for W25N01GV tRST (power-on and software reset) */
    stub_lib_delay_us(10000);

    ret = nand_wait_ready();
    if (ret != 0) {
        return ret;
    }

    s_nand_config.initialized = true;

    ret = nand_write_register(NAND_REG_PROTECT, 0x00);
    if (ret != 0) {
        return ret;
    }

    uint8_t prot_after = 0xFF;
    ret = nand_read_register(NAND_REG_PROTECT, &prot_after);
    if (ret != 0) {
        return NAND_ERR_SPI_FAIL;
    }
    if (prot_after != 0x00) {
        return NAND_ERR_PROTECTION;
    }

    /* Enable hardware ECC and Buffer Mode (BUF=1, ECC_EN=1, bits [4:3] of
     * NAND_REG_CONFIG).
     *
     * ECC (ECC_EN=1): the on-chip ECC engine covers columns 0–2047 (data
     * area). The spare area layout with ECC on:
     *   Bytes 0–3  : user Bad-Block-Marker (writable by the host, not touched
     *                by ECC engine — read_bbm/write_bbm only use these bytes).
     *   Bytes 4–63 : chip-managed ECC parity (computed during PROGRAM_EXECUTE,
     *                returned in READ_FROM_CACHE but writes to these bytes are
     *                silently ignored by the chip).
     * After each PAGE_READ the STATUS register bits [5:4] report the ECC
     * result: 00=clean, 01=1–4 bits corrected, 10=uncorrectable, 11=reserved.
     *
     * BUF=1 (Buffer Mode): disables auto-prefetch that exists in BUF=0
     * (Continuous Mode). In BUF=0, READ_FROM_CACHE completion triggers an
     * automatic PAGE_READ for the next sequential page. If a subsequent
     * explicit PAGE_READ arrives while the auto-prefetch is running (OIP=1),
     * the chip ignores it, leaving the OCA unloaded. All odd-block reads then
     * return the OCA power-on state (all zeros). BUF=1 avoids this entirely.
     *
     * In Buffer Mode the chip still has two cache registers — ECA for even
     * blocks (plane 0) and OCA for odd blocks (plane 1). CA[15] in
     * READ_FROM_CACHE, PROGRAM_LOAD, and PROGRAM_LOAD_RANDOM selects which
     * cache to access: CA[15]=0 → ECA, CA[15]=1 → OCA. */
    uint8_t cfg = 0;
    ret = nand_read_register(NAND_REG_CONFIG, &cfg);
    if (ret != 0) {
        return NAND_ERR_SPI_FAIL;
    }
    /* Enable ECC (ECC_EN=1) and Buffer Mode (BUF=1). */
    cfg = (uint8_t)(cfg | NAND_CFG_ECC_EN | NAND_CFG_BUF);
    ret = nand_write_register(NAND_REG_CONFIG, cfg);
    if (ret != 0) {
        return NAND_ERR_SPI_FAIL;
    }

    /* Verify CONFIG register: BUF=1 and ECC_EN=1 must both be set. */
    uint8_t cfg_after = 0;
    ret = nand_read_register(NAND_REG_CONFIG, &cfg_after);
    if (ret != 0) {
        return NAND_ERR_SPI_FAIL;
    }
    if ((cfg_after & NAND_CFG_BUF) != NAND_CFG_BUF) {
        return NAND_ERR_SPI_FAIL;
    }
    if ((cfg_after & NAND_CFG_ECC_EN) == 0) {
        return NAND_ERR_SPI_FAIL;
    }

    return 0;
}

int stub_target_nand_read_bbm(uint32_t page_number, uint8_t *spare_data)
{
    if (!s_nand_config.initialized) {
        return NAND_ERR_NOT_INITIALIZED;
    }

    if (!IS_ALIGNED((uintptr_t)spare_data, 4)) {
        return STUB_LIB_ERR_INVALID_ARG;
    }

    uint32_t page_addr = page_number & 0xFFFFFF;
    int ret;

    ret = spi_nand_transaction(NAND_CMD_PAGE_READ, page_addr, 24, NULL, 0, NULL, 0);
    if (ret != 0) {
        return NAND_ERR_SPI_FAIL;
    }

    ret = nand_wait_ready();
    if (ret != 0) {
        return ret;
    }

    /* Spare area starts at column page_size (2048).
     *
     * READ_FROM_CACHE (0x03) format: CMD(8) + CA_H(8) + CA_L(8) + DUMMY(8) + DATA.
     * Shift col_addr left by 8 so the 24-bit addr phase carries [CA_H, CA_L, 0x00]
     * with the trailing 0x00 serving as the required dummy byte. */
    uint32_t col_spare = (uint32_t)s_nand_config.page_size; /* 2048 */
    ret = spi_nand_transaction(NAND_CMD_READ_FROM_CACHE, col_spare << 8, 24, NULL, 0, spare_data, 32);
    if (ret != 0) {
        return NAND_ERR_SPI_FAIL;
    }
    return 0;
}

int stub_target_nand_write_bbm(uint32_t page_number, uint8_t is_bad)
{
    if (!s_nand_config.initialized) {
        return NAND_ERR_NOT_INITIALIZED;
    }

    uint8_t bad_block_marker[4];
    if (is_bad != 0) {
        bad_block_marker[0] = 0x00;
        bad_block_marker[1] = 0x00;
        bad_block_marker[2] = 0x00;
        bad_block_marker[3] = 0x00;
    } else {
        bad_block_marker[0] = 0xFF;
        bad_block_marker[1] = 0xFF;
        bad_block_marker[2] = 0xFF;
        bad_block_marker[3] = 0xFF;
    }

    uint32_t page_addr = page_number & 0xFFFFFF;

    int ret = spi_nand_transaction(NAND_CMD_PAGE_READ, page_addr, 24, NULL, 0, NULL, 0);
    if (ret != 0) {
        return NAND_ERR_SPI_FAIL;
    }

    ret = nand_wait_ready();
    if (ret != 0) {
        return ret;
    }

    ret = nand_write_enable();
    if (ret != 0) {
        return ret;
    }

    /* Column 2048 is the spare area.
     * Spare-area writes also require CA[15]=1 for odd blocks — same plane
     * selection rule as spare-area reads. */
    uint32_t block_number = page_number / s_nand_config.pages_per_block;
    uint32_t plane_bit = (block_number & 1u) ? 0x8000u : 0u;
    uint32_t col_addr = (uint32_t)s_nand_config.page_size | plane_bit;

    ret = spi_nand_transaction(NAND_CMD_PROGRAM_LOAD_RANDOM, col_addr, 16, bad_block_marker, 32, NULL, 0);
    if (ret != 0) {
        return NAND_ERR_SPI_FAIL;
    }

    ret = spi_nand_transaction(NAND_CMD_PROGRAM_EXECUTE, page_addr, 24, NULL, 0, NULL, 0);
    if (ret != 0) {
        return NAND_ERR_SPI_FAIL;
    }

    ret = nand_wait_program_erase();
    if (ret != 0) {
        return ret;
    }

    return 0;
}

#define SPI_NAND_MAX_RX_BYTES 64
#define SPI_NAND_MAX_TX_BYTES 64

int stub_target_nand_write_page(uint32_t page_number, const uint8_t *buf, uint32_t buf_size)
{
    if (!s_nand_config.initialized) {
        return NAND_ERR_NOT_INITIALIZED;
    }

    uint32_t write_len = MIN(buf_size, s_nand_config.page_size);

    if (write_len == 0) {
        return 0;
    }

    int ret = nand_write_enable();
    if (ret != 0) {
        return ret;
    }

    /* In Buffer Mode (BUF=1), CA[15] selects ECA (even blocks) or OCA (odd blocks). */
    uint32_t block_number = page_number / s_nand_config.pages_per_block;
    uint32_t plane_bit = (block_number & 1u) ? 0x8000u : 0u;

    uint32_t offset = 0;
    while (offset < write_len) {
        uint32_t chunk = write_len - offset;
        if (chunk > SPI_NAND_MAX_TX_BYTES) {
            chunk = SPI_NAND_MAX_TX_BYTES;
        }

        uint32_t col_addr = offset | plane_bit;

        uint8_t cmd = (offset == 0) ? NAND_CMD_PROGRAM_LOAD : NAND_CMD_PROGRAM_LOAD_RANDOM;
        ret = spi_nand_transaction(cmd, col_addr, 16, buf + offset, (uint16_t)(chunk * 8), NULL, 0);
        if (ret != 0) {
            return NAND_ERR_SPI_FAIL;
        }

        offset += chunk;
    }

    uint32_t page_addr = page_number & 0xFFFFFF;

    ret = spi_nand_transaction(NAND_CMD_PROGRAM_EXECUTE, page_addr, 24, NULL, 0, NULL, 0);
    if (ret != 0) {
        return NAND_ERR_SPI_FAIL;
    }

    ret = nand_wait_program_erase();
    if (ret != 0) {
        return ret;
    }

    return 0;
}

int stub_target_nand_erase_block(uint32_t page_number)
{
    if (!s_nand_config.initialized) {
        return NAND_ERR_NOT_INITIALIZED;
    }

    int ret = nand_write_enable();
    if (ret != 0) {
        return ret;
    }

    uint32_t page_addr = page_number & 0xFFFFFF;

    ret = spi_nand_transaction(NAND_CMD_ERASE_BLOCK, page_addr, 24, NULL, 0, NULL, 0);
    if (ret != 0) {
        return NAND_ERR_SPI_FAIL;
    }

    ret = nand_wait_program_erase();
    if (ret != 0) {
        return ret;
    }

    return 0;
}

/*
 * Check the ECC status bits [5:4] from the last status register read
 * (populated by nand_wait_ready() after a PAGE_READ command).
 *
 * W25N01GV ECC[1:0] field encoding:
 *   00 → clean (no errors)
 *   01 → 1–4 bit errors corrected (data valid, return 0)
 *   10 → uncorrectable (return NAND_ERR_ECC_UNCORRECTABLE)
 *   11 → reserved — treat the same as uncorrectable for safety
 *
 * Returns 0 on success (clean or corrected), NAND_ERR_ECC_UNCORRECTABLE otherwise.
 */
static int nand_check_ecc_after_read(void)
{
    uint8_t ecc_field = s_last_status_byte & NAND_STAT_ECC_MASK;
    /* Both 0x20 (10 = uncorrectable) and 0x30 (11 = reserved) are >= 0x20
     * and have bit5 set. Treat both as uncorrectable. */
    if (ecc_field >= NAND_STAT_ECC_UNCORR) {
        return NAND_ERR_ECC_UNCORRECTABLE;
    }
    return 0;
}

int stub_target_nand_read_page(uint32_t page_number, uint8_t *buf, uint32_t buf_size)
{
    if (!s_nand_config.initialized) {
        return NAND_ERR_NOT_INITIALIZED;
    }

    uint32_t read_len = buf_size;
    if (read_len > s_nand_config.page_size) {
        read_len = s_nand_config.page_size;
    }

    uint32_t page_addr = page_number & 0xFFFFFF;

    int ret = spi_nand_transaction(NAND_CMD_PAGE_READ, page_addr, 24, NULL, 0, NULL, 0);
    if (ret != 0) {
        return NAND_ERR_SPI_FAIL;
    }

    ret = nand_wait_ready();
    if (ret != 0) {
        return ret;
    }

    /* Check ECC status before reading data from cache. Fail fast on
     * uncorrectable errors — the cache may hold corrupted data. */
    ret = nand_check_ecc_after_read();
    if (ret != 0) {
        return ret;
    }

    /* In Buffer Mode (BUF=1), CA[15] selects ECA (even blocks) or OCA (odd blocks). */
    uint32_t block_number = page_number / s_nand_config.pages_per_block;
    uint32_t plane_bit = (block_number & 1u) ? 0x8000u : 0u;

    uint32_t offset = 0;
    while (offset < read_len) {
        uint32_t chunk = read_len - offset;
        if (chunk > SPI_NAND_MAX_RX_BYTES) {
            chunk = SPI_NAND_MAX_RX_BYTES;
        }

        uint32_t col_addr = offset | plane_bit;

        ret = spi_nand_transaction(NAND_CMD_READ_FROM_CACHE,
                                   col_addr << 8,
                                   24,
                                   NULL,
                                   0,
                                   buf + offset,
                                   (uint16_t)(chunk * 8));
        if (ret != 0) {
            return NAND_ERR_SPI_FAIL;
        }

        offset += chunk;
    }

    return 0;
}

uint32_t stub_target_nand_get_page_size(void)
{
    return s_nand_config.page_size;
}

uint32_t stub_target_nand_get_pages_per_block(void)
{
    return s_nand_config.pages_per_block;
}

uint32_t stub_target_nand_get_block_size(void)
{
    return s_nand_config.block_size;
}
