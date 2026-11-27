/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
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

static bool pin_valid(int pin)
{
    return pin >= 0 && pin < MAX_PAD_GPIO_NUM;
}

static void route_matrix_pin(uint8_t pin)
{
    esp_rom_gpio_pad_select_gpio(pin);
    gpio_pad_input_enable(pin);
    gpio_pad_set_drv(pin, 2);
}

int stub_target_nand_bus_init(const stub_lib_nand_pins_t *pins)
{
    int pin_clk = pins == NULL ? PIN_CLK : pins->clk;
    int pin_q = pins == NULL ? PIN_MISO : pins->q;
    int pin_d = pins == NULL ? PIN_MOSI : pins->d;
    int pin_cs = pins == NULL ? PIN_CS : pins->cs;
    int pin_hd = pins == NULL ? PIN_HD : pins->hd;
    int wp_pin = pins == NULL ? PIN_WP : pins->wp;

    // Validate pin numbers (ESP32-S3 GPIOs 0–47)
    if (!pin_valid(pin_clk) || !pin_valid(pin_q) || !pin_valid(pin_d) || !pin_valid(pin_cs) ||
        (pin_hd != STUB_LIB_NAND_PIN_NC && !pin_valid(pin_hd)) ||
        (wp_pin != STUB_LIB_NAND_PIN_NC && !pin_valid(wp_pin))) {
        return STUB_LIB_ERR_INVALID_ARG;
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

    bool use_iomux = (pin_clk == PIN_CLK && pin_q == PIN_MISO && pin_d == PIN_MOSI && pin_cs == PIN_CS &&
                      pin_hd == PIN_HD && wp_pin == PIN_WP);

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
        gpio_output_enable(PIN_MOSI);
        gpio_output_enable(PIN_CLK);
        gpio_output_enable(PIN_CS);
        gpio_output_enable(PIN_WP);
        gpio_output_enable(PIN_HD);
        gpio_output_disable(PIN_MISO);
    } else {
        // GPIO matrix path: set pins to GPIO, then route FSPI signals
        route_matrix_pin((uint8_t)pin_clk);
        route_matrix_pin((uint8_t)pin_q);
        route_matrix_pin((uint8_t)pin_d);
        route_matrix_pin((uint8_t)pin_cs);
        if (pin_hd != STUB_LIB_NAND_PIN_NC) {
            route_matrix_pin((uint8_t)pin_hd);
        }
        if (wp_pin != STUB_LIB_NAND_PIN_NC) {
            route_matrix_pin((uint8_t)wp_pin);
        }

        esp_rom_gpio_connect_out_signal((uint8_t)pin_clk, FSPICLK_OUT_IDX, false, false);
        esp_rom_gpio_connect_out_signal((uint8_t)pin_d, FSPID_OUT_IDX, false, false);
        esp_rom_gpio_connect_out_signal((uint8_t)pin_cs, FSPICS0_OUT_IDX, false, false);
        esp_rom_gpio_connect_in_signal((uint8_t)pin_q, FSPIQ_IN_IDX, false);
        if (pin_hd != STUB_LIB_NAND_PIN_NC) {
            esp_rom_gpio_connect_out_signal((uint8_t)pin_hd, FSPIHD_OUT_IDX, false, false);
            gpio_output_enable((uint8_t)pin_hd);
        }
        if (wp_pin != STUB_LIB_NAND_PIN_NC) {
            esp_rom_gpio_connect_out_signal((uint8_t)wp_pin, FSPIWP_OUT_IDX, false, false);
            gpio_output_enable((uint8_t)wp_pin);
        }

        gpio_output_enable((uint8_t)pin_clk);
        gpio_output_enable((uint8_t)pin_d);
        gpio_output_enable((uint8_t)pin_cs);
        gpio_output_disable((uint8_t)pin_q);
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
