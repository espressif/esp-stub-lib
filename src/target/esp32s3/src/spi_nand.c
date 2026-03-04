/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <string.h>

#include "spi_nand.h"

// SPI2 base address
#define SPI2_BASE                0x60024000

// SPI Register offsets (from ESP32-S3 TRM)
#define SPI_CMD_REG              (SPI2_BASE + 0x000)
#define SPI_ADDR_REG             (SPI2_BASE + 0x004)
#define SPI_CTRL_REG             (SPI2_BASE + 0x008)
#define SPI_CLOCK_REG            (SPI2_BASE + 0x00C)
#define SPI_USER_REG             (SPI2_BASE + 0x010)
#define SPI_USER1_REG            (SPI2_BASE + 0x014)
#define SPI_USER2_REG            (SPI2_BASE + 0x018)
#define SPI_MS_DLEN_REG          (SPI2_BASE + 0x01C)
#define SPI_MISC_REG             (SPI2_BASE + 0x020)
#define SPI_MOSI_DLEN_REG        (SPI2_BASE + 0x024)
#define SPI_MISO_DLEN_REG        (SPI2_BASE + 0x028)
#define SPI_DIN_MODE_REG         (SPI2_BASE + 0x024) /* same as MOSI_DLEN on this controller */
#define SPI_DIN_NUM_REG          (SPI2_BASE + 0x028) /* same as MISO_DLEN on this controller */
#define SPI_DOUT_MODE_REG        (SPI2_BASE + 0x02C)
#define SPI_DMA_CONF_REG         (SPI2_BASE + 0x030)
#define SPI_SLAVE_REG            (SPI2_BASE + 0x0E0)
#define SPI_CLK_GATE_REG         (SPI2_BASE + 0x0E8)
#define SPI_W0_REG               (SPI2_BASE + 0x098)

// System/GPIO registers
#define SYSTEM_PERIP_CLK_EN0_REG 0x600C0018U
#define SYSTEM_PERIP_RST_EN0_REG 0x600C001CU
#define IO_MUX_BASE              0x60009000U
#define GPIO_ENABLE_W1TC_REG     0x6000402CU
#define GPIO_ENABLE_W1TS_REG     0x60004024U

// System register bits
#define SYSTEM_SPI2_CLK_EN       (1 << 6)
#define SYSTEM_SPI2_RST          (1 << 6)

// SPI CMD register bits
#define SPI_USR                  (1 << 24)
#define SPI_UPDATE               (1 << 23)

// SPI CTRL register bits
#define SPI_WP_POL               (1 << 21)
#define SPI_HOLD_POL             (1 << 20)

// SPI USER register bits
#define SPI_USR_COMMAND          (1 << 31)
#define SPI_USR_ADDR             (1 << 30)
#define SPI_USR_MISO             (1 << 28)
#define SPI_USR_MOSI             (1 << 27)
#define SPI_CS_SETUP             (1 << 7)
#define SPI_CS_HOLD              (1 << 6)
#define SPI_DOUTDIN              (1 << 0)

// SPI CLK_GATE register bits
#define SPI_CLK_EN               (1 << 0)
#define SPI_MST_CLK_ACTIVE       (1 << 1)
#define SPI_MST_CLK_SEL          (1 << 2) // 0=XTAL, 1=PLL_CLK_80M

// SPI DMA_CONF register bits
#define SPI_RX_AFIFO_RST         (1U << 29)
#define SPI_BUF_AFIFO_RST        (1U << 30)

// Pin mappings (default IOMUX / native FSPI pins)
#define PIN_MOSI                 11
#define PIN_MISO                 13
#define PIN_CLK                  12
#define PIN_CS                   10
#define PIN_WP                   14
#define PIN_HD                   9

// FSPI GPIO matrix signal indices (ESP32-S3). Verify against ESP-IDF gpio_sig_map.h.
// If GPIO matrix path fails (e.g. nand_attach returns C400), try alternative set:
//   OUT: FSPICLK=103, FSPID=104, FSPICS0=106, FSPIHD=107, FSPIWP=108. IN: FSPIQ_IN=63.
#define FSPICLK_OUT_IDX          63
#define FSPIQ_IN_IDX             64
#define FSPID_OUT_IDX            65
#define FSPIHD_OUT_IDX           66
#define FSPIWP_OUT_IDX           67
#define FSPICS0_OUT_IDX          68

// Helper macros
#define REG_WRITE(_r, _v)        (*(volatile uint32_t *)(_r)) = (_v)
#define REG_READ(_r)             (*(volatile uint32_t *)(_r))
#define REG_SET_BIT(_r, _b)      REG_WRITE((_r), REG_READ(_r) | (uint32_t)(_b))
#define REG_CLR_BIT(_r, _b)      REG_WRITE((_r), REG_READ(_r) & ~(uint32_t)(_b))

// ROM functions
extern void esp_rom_delay_us(uint32_t us);
extern void esp_rom_gpio_pad_select_gpio(uint32_t gpio_num);
extern void esp_rom_gpio_connect_out_signal(uint32_t gpio_num, uint32_t signal_idx, bool out_inv, bool oen_inv);
extern void esp_rom_gpio_connect_in_signal(uint32_t gpio_num, uint32_t signal_idx, bool inv);

int spi_nand_init(uint32_t hspi_arg)
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
        REG_WRITE(GPIO_ENABLE_W1TS_REG,
                  (1U << pin_d) | (1U << pin_clk) | (1U << pin_cs) | (1U << PIN_WP) | (1U << pin_hd));
        REG_WRITE(GPIO_ENABLE_W1TC_REG, (1U << pin_q));
    } else {
        // GPIO matrix path: set pins to GPIO, then route FSPI signals
        uint8_t pins[] = { pin_clk, pin_q, pin_d, pin_cs, pin_hd };
        for (int i = 0; i < 5; i++) {
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
        if (pin_hd < 48) {
            esp_rom_gpio_connect_out_signal(pin_hd, FSPIHD_OUT_IDX, false, false);
        }

        uint32_t out_mask = (1U << pin_clk) | (1U << pin_d) | (1U << pin_cs);
        if (pin_hd < 48) {
            out_mask |= (1U << pin_hd);
        }
        REG_WRITE(GPIO_ENABLE_W1TS_REG, out_mask);
        REG_WRITE(GPIO_ENABLE_W1TC_REG, (1U << pin_q));
    }

    // SPI clock: PLL_CLK_80M / 8 = 10 MHz
    uint32_t clock_val = (7 << 0) | (3 << 6) | (7 << 12);
    REG_WRITE(SPI_CLOCK_REG, clock_val);

    // MISC: CS active low, CLK idle low
    REG_WRITE(SPI_MISC_REG, 0);

    // USER: full-duplex (DOUTDIN=1) so MISO reads from FSPIQ line
    REG_WRITE(SPI_USER_REG, SPI_CS_SETUP | SPI_CS_HOLD | SPI_DOUTDIN);

    // CS setup/hold time = 1 cycle each
    REG_WRITE(SPI_USER1_REG, (1U << 17) | (1U << 22));

    return 0;
}

int spi_nand_transaction(uint8_t cmd,
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
