/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

/*
 * Error convention: nand_* functions return negative values on failure.
 * Each function uses sequential offsets (-10, -20, -30, ...) as step markers
 * to identify which operation failed. Combined returns (e.g. -20 + ret)
 * encode both the step and the underlying error from a called function.
 * The host receives these as the 'value' field in the SLIP response.
 */

#include <stdbool.h>
#include <stddef.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/rom_wrappers.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/nand.h>

#include <soc/gpio_sig_map.h>
#include <soc/io_mux_reg.h>
#include <soc/reg_base.h>
#include <soc/soc_caps.h>
#include <soc/spi_reg.h>
#include <soc/system_reg.h>

// ---- SPI2 register definitions (via spi_reg.h parameterized macros) ----------

#define NAND_SPI_NUM        2

// Pin mappings (default IOMUX / native FSPI pins)
#define PIN_MOSI            11
#define PIN_MISO            13
#define PIN_CLK             12
#define PIN_CS              10
#define PIN_WP              14
#define PIN_HD              9

// IO_MUX pin register for a given GPIO number
#define IO_MUX_PIN_REG(pin) (PERIPHS_IO_MUX_GPIO0_U + (uint32_t)(pin) * 4U)

// ROM functions
extern void esp_rom_gpio_pad_select_gpio(uint32_t gpio_num);
extern void esp_rom_gpio_connect_out_signal(uint32_t gpio_num, uint32_t signal_idx, bool out_inv, bool oen_inv);
extern void esp_rom_gpio_connect_in_signal(uint32_t gpio_num, uint32_t signal_idx, bool inv);
extern void gpio_output_enable(uint32_t gpio_num);
extern void gpio_output_disable(uint32_t gpio_num);

// ---- SPI2 transport layer -------------------------------------------------

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
    if (pin_clk >= SOC_GPIO_PIN_COUNT || pin_q >= SOC_GPIO_PIN_COUNT || pin_d >= SOC_GPIO_PIN_COUNT ||
        pin_cs >= SOC_GPIO_PIN_COUNT || pin_hd >= SOC_GPIO_PIN_COUNT) {
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

    if (use_iomux) {
        // IO_MUX path: native FSPI pins (MCU_SEL=4)
        uint32_t all_pins[] = { pin_d, pin_q, pin_clk, pin_cs, PIN_WP, pin_hd };
        for (int i = 0; i < 6; i++) {
            uint32_t iomux_reg = IO_MUX_PIN_REG(all_pins[i]);
            uint32_t val = REG_READ(iomux_reg);
            val &= ~(uint32_t)(MCU_SEL_M);
            val |= (4U << MCU_SEL_S); // MCU_SEL = 4 (FSPI)
            val |= FUN_IE;
            val &= ~(uint32_t)(FUN_DRV_M);
            val |= (2U << FUN_DRV_S); // FUN_DRV = 2
            REG_WRITE(iomux_reg, val);
        }
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
            uint32_t iomux_reg = IO_MUX_PIN_REG(pins[i]);
            uint32_t val = REG_READ(iomux_reg);
            val |= FUN_IE;
            val &= ~(uint32_t)(FUN_DRV_M);
            val |= (2U << FUN_DRV_S); // FUN_DRV = 2
            REG_WRITE(iomux_reg, val);
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

    // SPI clock: PLL_CLK_80M / 2 = 40 MHz
    uint32_t clock_val = BIT(0) | (0U << 6) | BIT(12);
    REG_WRITE(SPI_CLOCK_REG(NAND_SPI_NUM), clock_val);

    // MISC: CS active low, CLK idle low
    REG_WRITE(SPI_MISC_REG(NAND_SPI_NUM), 0);

    // USER: full-duplex (DOUTDIN=1) so MISO reads from FSPIQ line
    REG_WRITE(SPI_USER_REG(NAND_SPI_NUM), SPI_CS_SETUP | SPI_CS_HOLD | SPI_DOUTDIN);

    // CS setup/hold time = 1 cycle each
    REG_WRITE(SPI_USER1_REG(NAND_SPI_NUM), (1U << SPI_CS_SETUP_TIME_S) | (1U << SPI_CS_HOLD_TIME_S));

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
    while (REG_READ(SPI_CMD_REG(NAND_SPI_NUM)) & SPI_USR) {
        stub_lib_delay_us(1);
    }

    // Reset FIFOs
    REG_SET_BIT(SPI_DMA_CONF_REG(NAND_SPI_NUM), SPI_BUF_AFIFO_RST);
    REG_CLR_BIT(SPI_DMA_CONF_REG(NAND_SPI_NUM), SPI_BUF_AFIFO_RST);
    REG_SET_BIT(SPI_DMA_CONF_REG(NAND_SPI_NUM), SPI_RX_AFIFO_RST);
    REG_CLR_BIT(SPI_DMA_CONF_REG(NAND_SPI_NUM), SPI_RX_AFIFO_RST);

    // Build USER register: full-duplex (DOUTDIN), command always present.
    uint32_t user_val = SPI_CS_SETUP | SPI_CS_HOLD | SPI_DOUTDIN | SPI_USR_COMMAND;

    if (addr_bits > 0) {
        user_val |= SPI_USR_ADDR;
    }
    if (tx_bits > 0 || rx_bits > 0) {
        user_val |= SPI_USR_MOSI;
    }
    if (rx_bits > 0) {
        user_val |= SPI_USR_MISO;
    }

    REG_WRITE(SPI_USER_REG(NAND_SPI_NUM), user_val);

    // 8-bit command
    REG_WRITE(SPI_USER2_REG(NAND_SPI_NUM), (uint32_t)cmd | ((uint32_t)(8U - 1U) << SPI_USR_COMMAND_BITLEN_S));

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
    while (REG_READ(SPI_CMD_REG(NAND_SPI_NUM)) & SPI_UPDATE) {
        stub_lib_delay_us(1);
    }

    REG_WRITE(SPI_CMD_REG(NAND_SPI_NUM), SPI_USR);
    while (REG_READ(SPI_CMD_REG(NAND_SPI_NUM)) & SPI_USR) {
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
 * Wait for NAND to be ready by polling the status register.
 *
 * Timeout derived from NAND_ERASE_TIMEOUT_US with 10us poll interval.
 * Worst-case timing per W25N01GV (Winbond, JEDEC ID EF:AA21) datasheet:
 *   block erase the max = 10ms, page program tPP max = 3ms,
 *   page read tRD max = 60us.
 *
 * Returns 0 on success, negative on error or timeout.
 */
static uint8_t s_last_status_byte;

static int nand_wait_ready(void)
{
    int timeout = (int)(NAND_ERASE_TIMEOUT_US / 10U);

    while (timeout-- > 0) {
        uint8_t status;

        int ret = spi_nand_transaction(NAND_CMD_READ_REGISTER, (uint32_t)NAND_REG_STATUS, 8, NULL, 0, &status, 8);
        if (ret != 0) {
            return ret;
        }

        s_last_status_byte = status;

        if ((status & NAND_STAT_BUSY) == 0) {
            if (status & NAND_STAT_ERASE_FAILED) {
                return NAND_ERR_ERASE_FAILED;
            }
            if (status & NAND_STAT_PROGRAM_FAILED) {
                return NAND_ERR_PROGRAM_FAILED;
            }
            return 0;
        }

        stub_lib_delay_us(10);
    }

    return NAND_ERR_TIMEOUT;
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
        return -(0x100 + s_last_status_byte);
    }

    s_nand_config.initialized = true;

    ret = nand_write_register(NAND_REG_PROTECT, 0x00);
    if (ret != 0) {
        return ret;
    }

    uint8_t prot_after = 0xFF;
    nand_read_register(NAND_REG_PROTECT, &prot_after);
    if (prot_after != 0x00) {
        return NAND_ERR_PROTECTION;
    }

    /* Disable hardware ECC (bit 4 of NAND_REG_CONFIG) so the host has raw access
     * to the spare area and ECC bytes do not overwrite bad-block markers. */
    uint8_t cfg = 0;
    nand_read_register(NAND_REG_CONFIG, &cfg);
    cfg = (uint8_t)(cfg & ~(uint8_t)0x10U); /* clear ECC_EN */
    nand_write_register(NAND_REG_CONFIG, cfg);

    return 0;
}

int stub_target_nand_read_bbm(uint32_t page_number, uint8_t *spare_data)
{
    if (!s_nand_config.initialized) {
        return NAND_ERR_NOT_INITIALIZED;
    }

    uint32_t page_addr = page_number & 0xFFFFFF;

    int ret = spi_nand_transaction(NAND_CMD_PAGE_READ, page_addr, 24, NULL, 0, NULL, 0);
    if (ret != 0) {
        return NAND_ERR_SPI_FAIL;
    }

    ret = nand_wait_ready();
    if (ret != 0) {
        return -20 + ret;
    }

    uint32_t col_addr = (uint32_t)s_nand_config.page_size;

    ret = spi_nand_transaction(NAND_CMD_READ_FROM_CACHE, col_addr << 8, 24, NULL, 0, spare_data, 16);
    if (ret != 0) {
        return -30;
    }

    return 0;
}

int stub_target_nand_write_bbm(uint32_t page_number, uint8_t is_bad)
{
    if (!s_nand_config.initialized) {
        return NAND_ERR_NOT_INITIALIZED;
    }

    uint8_t bad_block_marker[2];
    if (is_bad != 0) {
        bad_block_marker[0] = 0x00;
        bad_block_marker[1] = 0x00;
    } else {
        bad_block_marker[0] = 0xFF;
        bad_block_marker[1] = 0xFF;
    }

    int ret = nand_write_enable();
    if (ret != 0) {
        return ret;
    }

    uint32_t col_addr = (uint32_t)s_nand_config.page_size;

    ret = spi_nand_transaction(NAND_CMD_PROGRAM_LOAD_RANDOM, col_addr, 16, bad_block_marker, 16, NULL, 0);
    if (ret != 0) {
        return -20;
    }

    uint32_t page_addr = page_number & 0xFFFFFF;

    ret = spi_nand_transaction(NAND_CMD_PROGRAM_EXECUTE, page_addr, 24, NULL, 0, NULL, 0);
    if (ret != 0) {
        return -30;
    }

    ret = nand_wait_ready();
    if (ret != 0) {
        return -40 + ret;
    }

    return 0;
}

#define SPI_NAND_MAX_RX_BYTES 64
#define SPI_NAND_MAX_TX_BYTES 64

int stub_target_nand_write_page(uint32_t page_number, const uint8_t *buf)
{
    if (!s_nand_config.initialized) {
        return NAND_ERR_NOT_INITIALIZED;
    }

    int ret = nand_write_enable();
    if (ret != 0) {
        return ret;
    }

    uint32_t offset = 0;
    while (offset < s_nand_config.page_size) {
        uint32_t chunk = s_nand_config.page_size - offset;
        if (chunk > SPI_NAND_MAX_TX_BYTES) {
            chunk = SPI_NAND_MAX_TX_BYTES;
        }

        uint32_t col_addr = offset;

        uint8_t cmd = (offset == 0) ? NAND_CMD_PROGRAM_LOAD : NAND_CMD_PROGRAM_LOAD_RANDOM;
        ret = spi_nand_transaction(cmd, col_addr, 16, buf + offset, (uint16_t)(chunk * 8), NULL, 0);
        if (ret != 0) {
            return -20;
        }

        offset += chunk;
    }

    uint32_t page_addr = page_number & 0xFFFFFF;

    ret = spi_nand_transaction(NAND_CMD_PROGRAM_EXECUTE, page_addr, 24, NULL, 0, NULL, 0);
    if (ret != 0) {
        return -30;
    }

    stub_lib_delay_us(500);
    ret = nand_wait_ready();
    if (ret != 0) {
        return -40 + ret;
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

    ret = nand_wait_ready();
    if (ret != 0) {
        return -20 + ret;
    }

    stub_lib_delay_us(2000);
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
        return -20 + ret;
    }

    uint32_t offset = 0;
    while (offset < read_len) {
        uint32_t chunk = read_len - offset;
        if (chunk > SPI_NAND_MAX_RX_BYTES) {
            chunk = SPI_NAND_MAX_RX_BYTES;
        }

        uint32_t col_addr = offset;

        ret = spi_nand_transaction(NAND_CMD_READ_FROM_CACHE,
                                   col_addr << 8,
                                   24,
                                   NULL,
                                   0,
                                   buf + offset,
                                   (uint16_t)(chunk * 8));
        if (ret != 0) {
            return -30;
        }

        offset += chunk;
    }

    return 0;
}

uint32_t stub_target_nand_get_page_size(void)
{
    return s_nand_config.page_size;
}
