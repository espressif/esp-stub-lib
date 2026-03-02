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
#include <string.h>

#include <bit_utils.h>

#include <esp-stub-lib/soc_utils.h>

#include <target/nand.h>

#include <soc/system_reg.h>

// Delay function
extern void esp_rom_delay_us(uint32_t us);

// ---- SPI2 register definitions --------------------------------------------

// SPI2 base address
#define SPI2_BASE             0x60024000

// SPI Register offsets (from ESP32-S3 TRM)
#define SPI_CMD_REG           (SPI2_BASE + 0x000)
#define SPI_ADDR_REG          (SPI2_BASE + 0x004)
#define SPI_CTRL_REG          (SPI2_BASE + 0x008)
#define SPI_CLOCK_REG         (SPI2_BASE + 0x00C)
#define SPI_USER_REG          (SPI2_BASE + 0x010)
#define SPI_USER1_REG         (SPI2_BASE + 0x014)
#define SPI_USER2_REG         (SPI2_BASE + 0x018)
#define SPI_MS_DLEN_REG       (SPI2_BASE + 0x01C)
#define SPI_MISC_REG          (SPI2_BASE + 0x020)
#define SPI_MOSI_DLEN_REG     (SPI2_BASE + 0x024)
#define SPI_MISO_DLEN_REG     (SPI2_BASE + 0x028)
#define SPI_DIN_MODE_REG      (SPI2_BASE + 0x024) /* same as MOSI_DLEN on this controller */
#define SPI_DIN_NUM_REG       (SPI2_BASE + 0x028) /* same as MISO_DLEN on this controller */
#define SPI_DOUT_MODE_REG     (SPI2_BASE + 0x02C)
#define SPI_DMA_CONF_REG      (SPI2_BASE + 0x030)
#define SPI_SLAVE_REG         (SPI2_BASE + 0x0E0)
#define SPI_CLK_GATE_REG      (SPI2_BASE + 0x0E8)
#define SPI_W0_REG            (SPI2_BASE + 0x098)

// GPIO/IO_MUX registers
#define IO_MUX_BASE           0x60009000U
#define GPIO_ENABLE_W1TS_REG  0x60004024U
#define GPIO_ENABLE_W1TC_REG  0x6000402CU
#define GPIO_ENABLE1_W1TS_REG 0x60004034U
#define GPIO_ENABLE1_W1TC_REG 0x60004038U

// ESP32-S3 max GPIO number
#define ESP32S3_MAX_GPIO      48

// SPI CMD register bits
#define SPI_USR               (1 << 24)
#define SPI_UPDATE            (1 << 23)

// SPI CTRL register bits
#define SPI_WP_POL            (1 << 21)
#define SPI_HOLD_POL          (1 << 20)

// SPI USER register bits
#define SPI_USR_COMMAND       (1 << 31)
#define SPI_USR_ADDR          (1 << 30)
#define SPI_USR_MISO          (1 << 28)
#define SPI_USR_MOSI          (1 << 27)
#define SPI_CS_SETUP          (1 << 7)
#define SPI_CS_HOLD           (1 << 6)
#define SPI_DOUTDIN           (1 << 0)

// SPI CLK_GATE register bits
#define SPI_CLK_EN            (1 << 0)
#define SPI_MST_CLK_ACTIVE    (1 << 1)
#define SPI_MST_CLK_SEL       (1 << 2) // 0=XTAL, 1=PLL_CLK_80M

// SPI DMA_CONF register bits
#define SPI_RX_AFIFO_RST      (1U << 29)
#define SPI_BUF_AFIFO_RST     (1U << 30)

// Pin mappings (default IOMUX / native FSPI pins)
#define PIN_MOSI              11
#define PIN_MISO              13
#define PIN_CLK               12
#define PIN_CS                10
#define PIN_WP                14
#define PIN_HD                9

// FSPI GPIO matrix signal indices (ESP32-S3). Verify against ESP-IDF gpio_sig_map.h.
// If GPIO matrix path fails (e.g. nand_attach returns C400), try alternative set:
//   OUT: FSPICLK=103, FSPID=104, FSPICS0=106, FSPIHD=107, FSPIWP=108. IN: FSPIQ_IN=63.
#define FSPICLK_OUT_IDX       63
#define FSPIQ_IN_IDX          64
#define FSPID_OUT_IDX         65
#define FSPIHD_OUT_IDX        66
#define FSPIWP_OUT_IDX        67
#define FSPICS0_OUT_IDX       68

// ROM functions
extern void esp_rom_gpio_pad_select_gpio(uint32_t gpio_num);
extern void esp_rom_gpio_connect_out_signal(uint32_t gpio_num, uint32_t signal_idx, bool out_inv, bool oen_inv);
extern void esp_rom_gpio_connect_in_signal(uint32_t gpio_num, uint32_t signal_idx, bool inv);

// ---- GPIO helpers for pins >= 32 -------------------------------------------

static inline void gpio_enable_output(uint8_t pin)
{
    if (pin < 32) {
        REG_WRITE(GPIO_ENABLE_W1TS_REG, 1U << pin);
    } else {
        REG_WRITE(GPIO_ENABLE1_W1TS_REG, 1U << (pin - 32));
    }
}

static inline void gpio_disable_output(uint8_t pin)
{
    if (pin < 32) {
        REG_WRITE(GPIO_ENABLE_W1TC_REG, 1U << pin);
    } else {
        REG_WRITE(GPIO_ENABLE1_W1TC_REG, 1U << (pin - 32));
    }
}

// ---- SPI2 transport layer -------------------------------------------------

static int spi_nand_init(uint32_t hspi_arg)
{
    uint8_t pin_clk, pin_q, pin_d, pin_cs, pin_hd;

    if (hspi_arg == 0) {
        pin_clk = 12;
        pin_q = 13;
        pin_d = 11;
        pin_cs = 10;
        pin_hd = 9;
    } else {
        pin_clk = (uint8_t)((hspi_arg >> 0) & 0x3FU);
        pin_q = (uint8_t)((hspi_arg >> 6) & 0x3FU);
        pin_d = (uint8_t)((hspi_arg >> 12) & 0x3FU);
        pin_cs = (uint8_t)((hspi_arg >> 18) & 0x3FU);
        pin_hd = (uint8_t)((hspi_arg >> 24) & 0x3FU);
    }

    // Validate pin numbers (ESP32-S3 GPIOs 0–47)
    if (pin_clk >= ESP32S3_MAX_GPIO || pin_q >= ESP32S3_MAX_GPIO || pin_d >= ESP32S3_MAX_GPIO ||
        pin_cs >= ESP32S3_MAX_GPIO || pin_hd >= ESP32S3_MAX_GPIO) {
        return -10;
    }

    // Enable SPI2 peripheral clock and release reset
    REG_SET_BIT(SYSTEM_PERIP_CLK_EN0_REG, SYSTEM_SPI2_CLK_EN);
    REG_SET_BIT(SYSTEM_PERIP_RST_EN0_REG, SYSTEM_SPI2_RST);
    REG_CLR_BIT(SYSTEM_PERIP_RST_EN0_REG, SYSTEM_SPI2_RST);

    // Enable clock gate: PLL_CLK_80M source
    REG_WRITE(SPI_CLK_GATE_REG, SPI_CLK_EN | SPI_MST_CLK_ACTIVE | SPI_MST_CLK_SEL);

    // Master mode
    REG_WRITE(SPI_SLAVE_REG, 0);

    // Initialize registers — WP_POL and HOLD_POL must stay HIGH to avoid
    // activating the NAND chip's /HOLD and /WP (active-low) signals
    REG_WRITE(SPI_USER_REG, 0);
    REG_WRITE(SPI_USER1_REG, 0);
    REG_WRITE(SPI_USER2_REG, 0);
    REG_WRITE(SPI_CTRL_REG, SPI_WP_POL | SPI_HOLD_POL);
    REG_WRITE(SPI_CLOCK_REG, 0);
    REG_WRITE(SPI_MISC_REG, 0);
    REG_WRITE(SPI_MS_DLEN_REG, 0);
    REG_WRITE(SPI_DIN_MODE_REG, 0);
    REG_WRITE(SPI_DIN_NUM_REG, 0);
    REG_WRITE(SPI_DOUT_MODE_REG, 0);

    // Reset FIFOs
    REG_WRITE(SPI_DMA_CONF_REG, 0);
    REG_SET_BIT(SPI_DMA_CONF_REG, SPI_RX_AFIFO_RST);
    REG_CLR_BIT(SPI_DMA_CONF_REG, SPI_RX_AFIFO_RST);
    REG_SET_BIT(SPI_DMA_CONF_REG, SPI_BUF_AFIFO_RST);
    REG_CLR_BIT(SPI_DMA_CONF_REG, SPI_BUF_AFIFO_RST);

    bool use_iomux = (pin_clk == 12 && pin_q == 13 && pin_d == 11 && pin_cs == 10 && pin_hd == 9);

    if (use_iomux) {
        // IO_MUX path: native FSPI pins (MCU_SEL=4)
        uint32_t all_pins[] = { pin_d, pin_q, pin_clk, pin_cs, PIN_WP, pin_hd };
        for (int i = 0; i < 6; i++) {
            uint32_t iomux_reg = IO_MUX_BASE + all_pins[i] * 4;
            uint32_t val = REG_READ(iomux_reg);
            val &= ~(0x7U << 12);
            val |= (4U << 12); // MCU_SEL = 4 (FSPI)
            val |= (1U << 9);  // FUN_IE
            val &= ~(0x3U << 10);
            val |= (2U << 10); // FUN_DRV = 2
            REG_WRITE(iomux_reg, val);
        }
        gpio_enable_output(pin_d);
        gpio_enable_output(pin_clk);
        gpio_enable_output(pin_cs);
        gpio_enable_output(PIN_WP);
        gpio_enable_output(pin_hd);
        gpio_disable_output(pin_q);
    } else {
        // GPIO matrix path: set pins to GPIO, then route FSPI signals
        // Include PIN_WP so /WP is driven high (avoids floating write-protect)
        uint8_t pins[] = { pin_clk, pin_q, pin_d, pin_cs, pin_hd, PIN_WP };
        for (int i = 0; i < 6; i++) {
            esp_rom_gpio_pad_select_gpio(pins[i]);
            uint32_t iomux_reg = IO_MUX_BASE + (uint32_t)pins[i] * 4;
            uint32_t val = REG_READ(iomux_reg);
            val |= (1U << 9); // FUN_IE
            val &= ~(0x3U << 10);
            val |= (2U << 10); // FUN_DRV = 2
            REG_WRITE(iomux_reg, val);
        }

        esp_rom_gpio_connect_out_signal(pin_clk, FSPICLK_OUT_IDX, false, false);
        esp_rom_gpio_connect_out_signal(pin_d, FSPID_OUT_IDX, false, false);
        esp_rom_gpio_connect_out_signal(pin_cs, FSPICS0_OUT_IDX, false, false);
        esp_rom_gpio_connect_in_signal(pin_q, FSPIQ_IN_IDX, false);
        esp_rom_gpio_connect_out_signal(pin_hd, FSPIHD_OUT_IDX, false, false);
        esp_rom_gpio_connect_out_signal(PIN_WP, FSPIWP_OUT_IDX, false, false);

        gpio_enable_output(pin_clk);
        gpio_enable_output(pin_d);
        gpio_enable_output(pin_cs);
        gpio_enable_output(pin_hd);
        gpio_enable_output(PIN_WP);
        gpio_disable_output(pin_q);
    }

    // SPI clock: PLL_CLK_80M / 2 = 40 MHz
    uint32_t clock_val = (1 << 0) | (0 << 6) | (1 << 12);
    REG_WRITE(SPI_CLOCK_REG, clock_val);

    // MISC: CS active low, CLK idle low
    REG_WRITE(SPI_MISC_REG, 0);

    // USER: full-duplex (DOUTDIN=1) so MISO reads from FSPIQ line
    REG_WRITE(SPI_USER_REG, SPI_CS_SETUP | SPI_CS_HOLD | SPI_DOUTDIN);

    // CS setup/hold time = 1 cycle each
    REG_WRITE(SPI_USER1_REG, (1U << 17) | (1U << 22));

    return 0;
}

static int spi_nand_transaction(uint8_t cmd,
                                const uint8_t *addr,
                                uint8_t addr_bits,
                                const uint8_t *tx_data,
                                uint16_t tx_bits,
                                uint8_t *rx_data,
                                uint16_t rx_bits)
{
    while (REG_READ(SPI_CMD_REG) & SPI_USR) {
        esp_rom_delay_us(1);
    }

    // Reset FIFOs
    REG_SET_BIT(SPI_DMA_CONF_REG, SPI_BUF_AFIFO_RST);
    REG_CLR_BIT(SPI_DMA_CONF_REG, SPI_BUF_AFIFO_RST);
    REG_SET_BIT(SPI_DMA_CONF_REG, SPI_RX_AFIFO_RST);
    REG_CLR_BIT(SPI_DMA_CONF_REG, SPI_RX_AFIFO_RST);

    // Build USER register: full-duplex, command always present.
    // In full-duplex mode, MOSI must be enabled whenever MISO is active.
    uint32_t user_val = SPI_CS_SETUP | SPI_CS_HOLD | SPI_DOUTDIN | (uint32_t)SPI_USR_COMMAND;

    if (addr_bits > 0) {
        user_val |= SPI_USR_ADDR;
    }
    if (tx_bits > 0 || rx_bits > 0) {
        user_val |= SPI_USR_MOSI;
    }
    if (rx_bits > 0) {
        user_val |= SPI_USR_MISO;
    }

    REG_WRITE(SPI_USER_REG, user_val);

    // 8-bit command
    REG_WRITE(SPI_USER2_REG, (uint32_t)cmd | ((8U - 1U) << 28));

    // Address (MSB-aligned)
    if (addr_bits > 0) {
        uint32_t addr_val = 0;
        for (int i = 0; i < (int)((addr_bits + 7) / 8); i++) {
            addr_val = (addr_val << 8) | addr[i];
        }
        addr_val <<= (32 - addr_bits);
        REG_WRITE(SPI_ADDR_REG, addr_val);

        uint32_t user1_val = REG_READ(SPI_USER1_REG);
        user1_val &= ~(uint32_t)(0x1FU << 27);
        user1_val |= (uint32_t)((addr_bits - 1) << 27);
        REG_WRITE(SPI_USER1_REG, user1_val);
    }

    // Data phase (TX and RX share the same clock cycles in full-duplex)
    // ESP32-S3 has separate MOSI/MISO length registers; set both so RX phase length is correct.
    uint16_t data_bits = tx_bits > rx_bits ? tx_bits : rx_bits;

    if (data_bits > 0) {
        uint32_t data_bytes = (uint32_t)((data_bits + 7) / 8);
        for (uint32_t i = 0; i < (data_bytes + 3) / 4; i++) {
            uint32_t word = 0;
            if (tx_bits > 0 && tx_data != NULL) {
                for (uint32_t j = 0; j < 4 && (i * 4 + j) < (uint32_t)((tx_bits + 7) / 8); j++) {
                    word |= ((uint32_t)tx_data[i * 4 + j]) << (j * 8);
                }
            }
            REG_WRITE(SPI_W0_REG + (i * 4), word);
        }
        REG_WRITE(SPI_MS_DLEN_REG, (uint32_t)(data_bits - 1));
        REG_WRITE(SPI_MOSI_DLEN_REG, tx_bits > 0 ? (uint32_t)(tx_bits - 1) : 0);
        REG_WRITE(SPI_MISO_DLEN_REG, rx_bits > 0 ? (uint32_t)(rx_bits - 1) : 0);
    }

    // Apply config and start
    REG_WRITE(SPI_CMD_REG, SPI_UPDATE);
    while (REG_READ(SPI_CMD_REG) & SPI_UPDATE) {
        esp_rom_delay_us(1);
    }
    REG_WRITE(SPI_CMD_REG, SPI_USR);
    while (REG_READ(SPI_CMD_REG) & SPI_USR) {
        esp_rom_delay_us(1);
    }

    // Read RX data
    if (rx_bits > 0 && rx_data != NULL) {
        uint32_t rx_bytes = (uint32_t)((rx_bits + 7) / 8);
        for (uint32_t i = 0; i < (rx_bytes + 3) / 4; i++) {
            uint32_t word = REG_READ(SPI_W0_REG + (i * 4));
            for (uint32_t j = 0; j < 4 && (i * 4 + j) < rx_bytes; j++) {
                rx_data[i * 4 + j] = (uint8_t)((word >> (j * 8)) & 0xFF);
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
 * ~1 second timeout (100k iterations x 10us delay).
 * Worst-case timing per W25N01GV (Winbond, JEDEC ID EF:AA21) datasheet:
 *   block erase the max = 10ms, page program tPP max = 3ms,
 *   page read tRD max = 60us.
 * 1s provides ample margin for all operations.
 *
 * Returns 0 on success, negative on error or timeout.
 */
static uint8_t s_last_status_byte;

static int nand_wait_ready(void)
{
    int timeout = 100000;

    while (timeout-- > 0) {
        uint8_t status;
        uint8_t reg_addr = REG_STATUS;

        int ret = spi_nand_transaction(CMD_READ_REGISTER, &reg_addr, 8, NULL, 0, &status, 8);
        if (ret != 0) {
            return ret;
        }

        s_last_status_byte = status;

        if ((status & STAT_BUSY) == 0) {
            if (status & STAT_ERASE_FAILED) {
                return -2; /* erase failed */
            }
            if (status & STAT_PROGRAM_FAILED) {
                return -3; /* program failed */
            }
            return 0;
        }

        esp_rom_delay_us(10);
    }

    return -5; /* timeout */
}

/**
 * @brief Read NAND register
 */
static int nand_read_register(uint8_t reg, uint8_t *val) __attribute__((unused));
static int nand_read_register(uint8_t reg, uint8_t *val)
{
    return spi_nand_transaction(CMD_READ_REGISTER, &reg, 8, NULL, 0, val, 8);
}

/**
 * @brief Write NAND register
 */
static int nand_write_register(uint8_t reg, uint8_t val)
{
    uint8_t data[2] = { reg, val };
    return spi_nand_transaction(CMD_SET_REGISTER, NULL, 0, data, 16, NULL, 0);
}

/**
 * @brief Issue write enable command
 */
static int nand_write_enable(void)
{
    return spi_nand_transaction(CMD_WRITE_ENABLE, NULL, 0, NULL, 0, NULL, 0);
}

int stub_target_nand_read_id(uint8_t *manufacturer_id, uint16_t *device_id)
{
    uint8_t dummy = 0x00;
    uint8_t id_buf[3] = { 0 };
    int ret = spi_nand_transaction(CMD_READ_ID, &dummy, 8, NULL, 0, id_buf, 24);
    if (ret != 0) {
        return ret;
    }
    if (manufacturer_id) {
        *manufacturer_id = id_buf[0];
    }
    if (device_id) {
        *device_id = (uint16_t)(((uint16_t)id_buf[1] << 8) | (uint16_t)id_buf[2]);
    }
    return 0;
}

static uint8_t s_debug_id[3] = { 0 };
static uint8_t s_debug_extra[3] = { 0 };

int stub_target_nand_attach(uint32_t hspi_arg)
{
    int ret;

    s_nand_config.page_size = 2048;
    s_nand_config.pages_per_block = 64;
    s_nand_config.block_size = 128 * 1024;
    s_nand_config.initialized = false;
    s_last_status_byte = 0xFF;

    ret = spi_nand_init(hspi_arg);
    if (ret != 0) {
        return ret;
    }

    /* Allow SPI2 peripheral to stabilize after clock enable and reset release */
    esp_rom_delay_us(5000);

    /* Device reset; W25N01GV datasheet tRST max = 500us for power-on reset */
    ret = spi_nand_transaction(CMD_RESET, NULL, 0, NULL, 0, NULL, 0);
    if (ret != 0) {
        return -100; /* reset command failed */
    }

    /* Wait 10ms after reset for W25N01GV tRST (power-on and software reset) */
    esp_rom_delay_us(10000);

    ret = nand_wait_ready();
    if (ret != 0) {
        return -(0x100 + s_last_status_byte);
    }

    s_nand_config.initialized = true;

    uint8_t dummy = 0x00;
    spi_nand_transaction(CMD_READ_ID, &dummy, 8, NULL, 0, s_debug_id, 24);

    uint8_t status = 0xFF;
    uint8_t reg_addr = REG_STATUS;
    spi_nand_transaction(CMD_READ_REGISTER, &reg_addr, 8, NULL, 0, &status, 8);
    s_debug_extra[0] = status;

    reg_addr = REG_PROTECT;
    spi_nand_transaction(CMD_READ_REGISTER, &reg_addr, 8, NULL, 0, &status, 8);
    s_debug_extra[1] = status;

    reg_addr = REG_CONFIG;
    spi_nand_transaction(CMD_READ_REGISTER, &reg_addr, 8, NULL, 0, &status, 8);
    s_debug_extra[2] = status;

    ret = nand_write_register(REG_PROTECT, 0x00);
    if (ret != 0) {
        return ret;
    }

    uint8_t prot_after = 0xFF;
    nand_read_register(REG_PROTECT, &prot_after);
    s_debug_extra[1] = prot_after;
    if (prot_after != 0x00) {
        return -50; /* protection register write failed */
    }

    return 0;
}

int stub_target_nand_read_spare(uint32_t page_number, uint8_t *spare_data)
{
    if (!s_nand_config.initialized) {
        return -1;
    }

    uint8_t page_addr[3];
    page_addr[0] = (uint8_t)((page_number >> 16) & 0xFF);
    page_addr[1] = (uint8_t)((page_number >> 8) & 0xFF);
    page_addr[2] = (uint8_t)(page_number & 0xFF);

    int ret = spi_nand_transaction(CMD_PAGE_READ, page_addr, 24, NULL, 0, NULL, 0);
    if (ret != 0) {
        return -10; /* PAGE_READ command failed */
    }

    ret = nand_wait_ready();
    if (ret != 0) {
        return -20 + ret; /* wait_ready after PAGE_READ */
    }

    uint16_t column = (uint16_t)s_nand_config.page_size;
    uint8_t col_addr[3];
    col_addr[0] = (uint8_t)((column >> 8) & 0xFF);
    col_addr[1] = (uint8_t)(column & 0xFF);
    col_addr[2] = 0;

    ret = spi_nand_transaction(CMD_READ_FROM_CACHE, col_addr, 24, NULL, 0, spare_data, 16);
    if (ret != 0) {
        return -30; /* READ_FROM_CACHE command failed */
    }

    return 0;
}

int stub_target_nand_write_spare(uint32_t page_number, uint8_t is_bad)
{
    if (!s_nand_config.initialized) {
        return -1;
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

    uint16_t column = (uint16_t)s_nand_config.page_size;
    uint8_t col_addr[2];
    col_addr[0] = (uint8_t)((column >> 8) & 0xFFU);
    col_addr[1] = (uint8_t)(column & 0xFFU);

    ret = spi_nand_transaction(CMD_PROGRAM_LOAD_RANDOM, col_addr, 16, bad_block_marker, 16, NULL, 0);
    if (ret != 0) {
        return -20; /* PROGRAM_LOAD_RANDOM command failed */
    }

    uint8_t page_addr[3];
    page_addr[0] = (uint8_t)((page_number >> 16) & 0xFF);
    page_addr[1] = (uint8_t)((page_number >> 8) & 0xFF);
    page_addr[2] = (uint8_t)(page_number & 0xFF);

    ret = spi_nand_transaction(CMD_PROGRAM_EXECUTE, page_addr, 24, NULL, 0, NULL, 0);
    if (ret != 0) {
        return -30; /* PROGRAM_EXECUTE command failed */
    }

    ret = nand_wait_ready();
    if (ret != 0) {
        return -40 + ret; /* wait_ready after PROGRAM_EXECUTE */
    }

    return 0;
}

#define SPI_NAND_MAX_RX_BYTES 64
#define SPI_NAND_MAX_TX_BYTES 64

int stub_target_nand_write_page(uint32_t page_number, const uint8_t *buf)
{
    if (!s_nand_config.initialized) {
        return -1;
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

        uint16_t column = (uint16_t)offset;
        uint8_t col_addr[2];
        col_addr[0] = (uint8_t)((column >> 8) & 0xFF);
        col_addr[1] = (uint8_t)(column & 0xFF);

        uint8_t cmd = (offset == 0) ? CMD_PROGRAM_LOAD : CMD_PROGRAM_LOAD_RANDOM;
        ret = spi_nand_transaction(cmd, col_addr, 16, buf + offset, (uint16_t)(chunk * 8), NULL, 0);
        if (ret != 0) {
            return -20; /* PROGRAM_LOAD/PROGRAM_LOAD_RANDOM command failed */
        }

        offset += chunk;
    }

    uint8_t page_addr[3];
    page_addr[0] = (uint8_t)((page_number >> 16) & 0xFF);
    page_addr[1] = (uint8_t)((page_number >> 8) & 0xFF);
    page_addr[2] = (uint8_t)(page_number & 0xFF);

    ret = spi_nand_transaction(CMD_PROGRAM_EXECUTE, page_addr, 24, NULL, 0, NULL, 0);
    if (ret != 0) {
        return -30; /* PROGRAM_EXECUTE command failed */
    }

    esp_rom_delay_us(500);
    ret = nand_wait_ready();
    if (ret != 0) {
        return -40 + ret; /* wait_ready after PROGRAM_EXECUTE */
    }

    return 0;
}

int stub_target_nand_erase_block(uint32_t page_number)
{
    if (!s_nand_config.initialized) {
        return -1;
    }

    int ret = nand_write_enable();
    if (ret != 0) {
        return ret;
    }

    uint8_t page_addr[3];
    page_addr[0] = (uint8_t)((page_number >> 16) & 0xFF);
    page_addr[1] = (uint8_t)((page_number >> 8) & 0xFF);
    page_addr[2] = (uint8_t)(page_number & 0xFF);

    ret = spi_nand_transaction(CMD_ERASE_BLOCK, page_addr, 24, NULL, 0, NULL, 0);
    if (ret != 0) {
        return -10; /* ERASE_BLOCK command failed */
    }

    ret = nand_wait_ready();
    if (ret != 0) {
        return -20 + ret; /* wait_ready after ERASE_BLOCK */
    }

    esp_rom_delay_us(2000);
    return 0;
}

int stub_target_nand_read_page(uint32_t page_number, uint8_t *buf, uint32_t buf_size)
{
    if (!s_nand_config.initialized) {
        return -1;
    }

    uint32_t read_len = buf_size;
    if (read_len > s_nand_config.page_size) {
        read_len = s_nand_config.page_size;
    }

    uint8_t page_addr[3];
    page_addr[0] = (uint8_t)((page_number >> 16) & 0xFF);
    page_addr[1] = (uint8_t)((page_number >> 8) & 0xFF);
    page_addr[2] = (uint8_t)(page_number & 0xFF);

    int ret = spi_nand_transaction(CMD_PAGE_READ, page_addr, 24, NULL, 0, NULL, 0);
    if (ret != 0) {
        return -10; /* PAGE_READ command failed */
    }

    ret = nand_wait_ready();
    if (ret != 0) {
        return -20 + ret; /* wait_ready after PAGE_READ */
    }

    uint32_t offset = 0;
    while (offset < read_len) {
        uint32_t chunk = read_len - offset;
        if (chunk > SPI_NAND_MAX_RX_BYTES) {
            chunk = SPI_NAND_MAX_RX_BYTES;
        }

        uint16_t column = (uint16_t)offset;
        uint8_t col_addr[3];
        col_addr[0] = (uint8_t)((column >> 8) & 0xFF);
        col_addr[1] = (uint8_t)(column & 0xFF);
        col_addr[2] = 0x00;

        ret = spi_nand_transaction(CMD_READ_FROM_CACHE, col_addr, 24, NULL, 0, buf + offset, (uint16_t)(chunk * 8));
        if (ret != 0) {
            return -30; /* READ_FROM_CACHE command failed */
        }

        offset += chunk;
    }

    return 0;
}

uint32_t stub_target_nand_get_page_size(void)
{
    return s_nand_config.page_size;
}

const uint8_t *stub_target_nand_get_debug_id(void)
{
    return s_debug_id;
}

const uint8_t *stub_target_nand_get_debug_extra(void)
{
    return s_debug_extra;
}
