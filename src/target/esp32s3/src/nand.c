/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/err.h>
#include <esp-stub-lib/rom_wrappers.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/nand.h>

#include <soc/gpio_sig_map.h>
#include <soc/io_mux_reg.h>
#include <soc/reg_base.h>
#include <soc/spi_reg.h>
#include <soc/system_reg.h>

#define NAND_SPI_NUM 2

// Pin mappings (default IOMUX / native FSPI pins)
#define PIN_MOSI     11
#define PIN_MISO     13
#define PIN_CLK      12
#define PIN_CS       10
#define PIN_WP       14
#define PIN_HD       9

// ROM functions
extern void esp_rom_gpio_pad_select_gpio(uint32_t gpio_num);
extern void esp_rom_gpio_connect_out_signal(uint32_t gpio_num, uint32_t signal_idx, bool out_inv, bool oen_inv);
extern void esp_rom_gpio_connect_in_signal(uint32_t gpio_num, uint32_t signal_idx, bool inv);
extern void gpio_output_enable(uint32_t gpio_num);
extern void gpio_output_disable(uint32_t gpio_num);
extern void gpio_pad_set_drv(uint32_t gpio_num, uint32_t drv);
extern void gpio_pad_input_enable(uint32_t gpio_num);

// ---- SPI2 transport layer -------------------------------------------------

#define SPI_BUSY_TIMEOUT_ITERS 100000

static int spi_nand_init(uint32_t hspi_arg)
{
    uint8_t pin_clk, pin_q, pin_d, pin_cs, pin_hd;

    if (hspi_arg == 0) {
        pin_clk = PIN_CLK;
        pin_q = PIN_MISO;
        pin_d = PIN_MOSI;
        pin_cs = PIN_CS;
        pin_hd = PIN_HD;
    } else {
        pin_clk = HSPI_PIN_FIELD(hspi_arg, 0);
        pin_q = HSPI_PIN_FIELD(hspi_arg, 6);
        pin_d = HSPI_PIN_FIELD(hspi_arg, 12);
        pin_cs = HSPI_PIN_FIELD(hspi_arg, 18);
        pin_hd = HSPI_PIN_FIELD(hspi_arg, 24);
    }

    // Validate pin numbers (ESP32-S3 GPIOs 0–47)
    if (pin_clk >= MAX_PAD_GPIO_NUM || pin_q >= MAX_PAD_GPIO_NUM || pin_d >= MAX_PAD_GPIO_NUM ||
        pin_cs >= MAX_PAD_GPIO_NUM || pin_hd >= MAX_PAD_GPIO_NUM) {
        return NAND_ERR_PIN_INVALID;
    }

    // Enable SPI2 peripheral clock and release reset
    REG_SET_BIT(SYSTEM_PERIP_CLK_EN0_REG, SYSTEM_SPI2_CLK_EN);
    REG_SET_BIT(SYSTEM_PERIP_RST_EN0_REG, SYSTEM_SPI2_RST);
    REG_CLR_BIT(SYSTEM_PERIP_RST_EN0_REG, SYSTEM_SPI2_RST);

    // Enable clock gate: PLL_CLK_80M source
    REG_WRITE(SPI_CLK_GATE_REG(NAND_SPI_NUM), SPI_CLK_EN | SPI_MST_CLK_ACTIVE | SPI_MST_CLK_SEL);

    // Master mode
    REG_WRITE(SPI_SLAVE_REG(NAND_SPI_NUM), 0);

    // Initialize registers — WP_POL and HOLD_POL must stay HIGH to avoid
    // activating the NAND chip's /HOLD and /WP (active-low) signals
    REG_WRITE(SPI_USER_REG(NAND_SPI_NUM), 0);
    REG_WRITE(SPI_USER1_REG(NAND_SPI_NUM), 0);
    REG_WRITE(SPI_USER2_REG(NAND_SPI_NUM), 0);
    REG_WRITE(SPI_CTRL_REG(NAND_SPI_NUM), SPI_WP_POL | SPI_HOLD_POL);
    REG_WRITE(SPI_CLOCK_REG(NAND_SPI_NUM), 0);
    REG_WRITE(SPI_MISC_REG(NAND_SPI_NUM), 0);
    REG_WRITE(SPI_MS_DLEN_REG(NAND_SPI_NUM), 0);
    REG_WRITE(SPI_DIN_MODE_REG(NAND_SPI_NUM), 0);
    REG_WRITE(SPI_DIN_NUM_REG(NAND_SPI_NUM), 0);
    REG_WRITE(SPI_DOUT_MODE_REG(NAND_SPI_NUM), 0);

    // Reset FIFOs
    REG_WRITE(SPI_DMA_CONF_REG(NAND_SPI_NUM), 0);
    REG_SET_BIT(SPI_DMA_CONF_REG(NAND_SPI_NUM), SPI_RX_AFIFO_RST);
    REG_CLR_BIT(SPI_DMA_CONF_REG(NAND_SPI_NUM), SPI_RX_AFIFO_RST);
    REG_SET_BIT(SPI_DMA_CONF_REG(NAND_SPI_NUM), SPI_BUF_AFIFO_RST);
    REG_CLR_BIT(SPI_DMA_CONF_REG(NAND_SPI_NUM), SPI_BUF_AFIFO_RST);

    bool use_iomux =
        (pin_clk == PIN_CLK && pin_q == PIN_MISO && pin_d == PIN_MOSI && pin_cs == PIN_CS && pin_hd == PIN_HD);

// Set MCU_SEL=4 (FSPI) on one pin using its named IO_MUX register constant.
// Defined as a local macro to avoid array initializers, which the compiler may
// place in .rodata — mapped into .data by the plugin linker script and rejected.
#define SET_IOMUX_FSPI(iomux_reg, gpio_num)                                                                            \
    do {                                                                                                               \
        uint32_t _v = REG_READ(iomux_reg);                                                                             \
        _v = (_v & ~(uint32_t)(MCU_SEL_M)) | (4U << MCU_SEL_S);                                                        \
        REG_WRITE((iomux_reg), _v);                                                                                    \
        gpio_pad_input_enable(gpio_num);                                                                               \
        gpio_pad_set_drv((gpio_num), 2);                                                                               \
    } while (0)

    if (use_iomux) {
        // IO_MUX path: native FSPI pins (MCU_SEL=4)
        // Each pin uses its named IO_MUX register constant — no computed addressing.
        SET_IOMUX_FSPI(PERIPHS_IO_MUX_GPIO11_U, PIN_MOSI);
        SET_IOMUX_FSPI(PERIPHS_IO_MUX_GPIO13_U, PIN_MISO);
        SET_IOMUX_FSPI(PERIPHS_IO_MUX_GPIO12_U, PIN_CLK);
        SET_IOMUX_FSPI(PERIPHS_IO_MUX_GPIO10_U, PIN_CS);
        SET_IOMUX_FSPI(PERIPHS_IO_MUX_GPIO14_U, PIN_WP);
        SET_IOMUX_FSPI(PERIPHS_IO_MUX_GPIO9_U, PIN_HD);
        gpio_output_enable(pin_d);
        gpio_output_enable(pin_clk);
        gpio_output_enable(pin_cs);
        gpio_output_enable(PIN_WP);
        gpio_output_enable(pin_hd);
        gpio_output_disable(pin_q);
    } else {
        // GPIO matrix path: set pins to GPIO, then route FSPI signals
        // Include PIN_WP so /WP is driven high (avoids floating write-protect)
        uint8_t pins[] = { pin_clk, pin_q, pin_d, pin_cs, pin_hd, PIN_WP };
        for (int i = 0; i < 6; i++) {
            esp_rom_gpio_pad_select_gpio(pins[i]);
            gpio_pad_input_enable(pins[i]);
            gpio_pad_set_drv(pins[i], 2);
        }

        esp_rom_gpio_connect_out_signal(pin_clk, FSPICLK_OUT_IDX, false, false);
        esp_rom_gpio_connect_out_signal(pin_d, FSPID_OUT_IDX, false, false);
        esp_rom_gpio_connect_out_signal(pin_cs, FSPICS0_OUT_IDX, false, false);
        esp_rom_gpio_connect_in_signal(pin_q, FSPIQ_IN_IDX, false);
        esp_rom_gpio_connect_out_signal(pin_hd, FSPIHD_OUT_IDX, false, false);
        esp_rom_gpio_connect_out_signal(PIN_WP, FSPIWP_OUT_IDX, false, false);

        gpio_output_enable(pin_clk);
        gpio_output_enable(pin_d);
        gpio_output_enable(pin_cs);
        gpio_output_enable(pin_hd);
        gpio_output_enable(PIN_WP);
        gpio_output_disable(pin_q);
    }
#undef SET_IOMUX_FSPI

    // SPI clock: PLL_CLK_80M / 2 = 40 MHz (CLKCNT_N=1, CLKCNT_H=0, CLKCNT_L=1)
    REG_WRITE(SPI_CLOCK_REG(NAND_SPI_NUM), (1U << SPI_CLKCNT_N_S) | (1U << SPI_CLKCNT_L_S));

    // MISC: CS active low, CLK idle low
    REG_WRITE(SPI_MISC_REG(NAND_SPI_NUM), 0);

    // USER: full-duplex (DOUTDIN=1) so MISO reads from FSPIQ line
    REG_WRITE(SPI_USER_REG(NAND_SPI_NUM), SPI_CS_SETUP | SPI_CS_HOLD | SPI_DOUTDIN);

    // CS setup/hold time = 1 cycle each
    REG_SET_FIELD(SPI_USER1_REG(NAND_SPI_NUM), SPI_CS_SETUP_TIME, 1U);
    REG_SET_FIELD(SPI_USER1_REG(NAND_SPI_NUM), SPI_CS_HOLD_TIME, 1U);

    return 0;
}

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

int stub_target_nand_attach(uint32_t hspi_arg)
{
    /* W25N01GVZEIG geometry — hardcoded as this is the only supported NAND chip.
     * Future multi-chip support should read JEDEC parameter pages instead. */
    s_nand_config.page_size = 2048;
    s_nand_config.pages_per_block = 64;
    s_nand_config.block_size = 128 * 1024;
    s_nand_config.initialized = false;
    s_last_status_byte = 0xFF;

    int ret = spi_nand_init(hspi_arg);
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
