/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * ESP32-P4 GPSPI2 bring-up for SPI NAND. The SPI transaction lives in
 * nand_gpspi.c. Revision-specific linker aliases map the GPIO calls to the
 * corresponding rev1 or ECO5 ROM entry points.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/nand.h>

#include <soc/gpio_sig_map.h>
#include <soc/hp_sys_clkrst_reg.h>
#include <soc/reg_base.h>
#include <soc/soc_caps.h>
#include <soc/spi_reg.h>

#define NAND_SPI_NUM 2

/* Same default pin numbers as the ESP32-S3 FSPI bus. P4 has no native
 * function on these pads, so every pin goes through the GPIO matrix. */
#define PIN_MOSI     11
#define PIN_MISO     13
#define PIN_CLK      12
#define PIN_CS       10
#define PIN_HD       9
#define PIN_WP       14

extern void esp_rom_gpio_pad_select_gpio(uint32_t gpio_num);
extern void esp_rom_gpio_pad_input_enable(uint32_t gpio_num);
extern void esp_rom_gpio_pad_set_drv(uint32_t gpio_num, uint32_t drv);
extern void esp_rom_gpio_set_output_level(uint32_t gpio_num, uint32_t level);
extern void esp_rom_gpio_output_enable(uint32_t gpio_num);
extern void esp_rom_gpio_output_disable(uint32_t gpio_num);
extern void esp_rom_gpio_connect_out_signal(uint32_t gpio_num, uint32_t signal_idx, bool out_inv, bool oen_inv);
extern void esp_rom_gpio_connect_in_signal(uint32_t gpio_num, uint32_t signal_idx, bool inv);

static void route_pin(uint8_t pin)
{
    esp_rom_gpio_pad_select_gpio(pin);
    esp_rom_gpio_pad_input_enable(pin);
    esp_rom_gpio_pad_set_drv(pin, 2);
}

static bool pin_valid(int pin)
{
    return pin >= 0 && pin < SOC_GPIO_PIN_COUNT;
}

int stub_target_nand_bus_init(const stub_lib_nand_pins_t *pins)
{
    int pin_clk = pins == NULL ? PIN_CLK : pins->clk;
    int pin_q = pins == NULL ? PIN_MISO : pins->q;
    int pin_d = pins == NULL ? PIN_MOSI : pins->d;
    int pin_cs = pins == NULL ? PIN_CS : pins->cs;
    int pin_hd = pins == NULL ? PIN_HD : pins->hd;
    int wp_pin = pins == NULL ? PIN_WP : pins->wp;

    if (!pin_valid(pin_clk) || !pin_valid(pin_q) || !pin_valid(pin_d) || !pin_valid(pin_cs) ||
        (pin_hd != STUB_LIB_NAND_PIN_NC && !pin_valid(pin_hd)) ||
        (wp_pin != STUB_LIB_NAND_PIN_NC && !pin_valid(wp_pin))) {
        return STUB_LIB_ERR_INVALID_ARG;
    }

    REG_SET_BIT(HP_SYS_CLKRST_SOC_CLK_CTRL1_REG, HP_SYS_CLKRST_REG_GPSPI2_SYS_CLK_EN);
    REG_SET_BIT(HP_SYS_CLKRST_SOC_CLK_CTRL2_REG, HP_SYS_CLKRST_REG_GPSPI2_APB_CLK_EN);
    REG_SET_BIT(HP_SYS_CLKRST_HP_RST_EN2_REG, HP_SYS_CLKRST_REG_RST_EN_SPI2);
    REG_CLR_BIT(HP_SYS_CLKRST_HP_RST_EN2_REG, HP_SYS_CLKRST_REG_RST_EN_SPI2);
    /* Source 0 is XTAL (40 MHz). Enable the high-speed and master gates. */
    REG_SET_BIT(HP_SYS_CLKRST_PERI_CLK_CTRL116_REG,
                HP_SYS_CLKRST_REG_GPSPI2_HS_CLK_EN | HP_SYS_CLKRST_REG_GPSPI2_MST_CLK_EN);

    /* SPI_MST_CLK_SEL = 0 keeps the module on XTAL rather than PLL_CLK_80M. */
    REG_WRITE(SPI_CLK_GATE_REG(NAND_SPI_NUM), SPI_CLK_EN | SPI_MST_CLK_ACTIVE);
    REG_WRITE(SPI_SLAVE_REG(NAND_SPI_NUM), 0);
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

    REG_WRITE(SPI_DMA_CONF_REG(NAND_SPI_NUM), 0);
    REG_SET_BIT(SPI_DMA_CONF_REG(NAND_SPI_NUM), SPI_RX_AFIFO_RST);
    REG_CLR_BIT(SPI_DMA_CONF_REG(NAND_SPI_NUM), SPI_RX_AFIFO_RST);
    REG_SET_BIT(SPI_DMA_CONF_REG(NAND_SPI_NUM), SPI_BUF_AFIFO_RST);
    REG_CLR_BIT(SPI_DMA_CONF_REG(NAND_SPI_NUM), SPI_BUF_AFIFO_RST);

    route_pin((uint8_t)pin_clk);
    route_pin((uint8_t)pin_q);
    route_pin((uint8_t)pin_d);
    route_pin((uint8_t)pin_cs);
    if (pin_hd != STUB_LIB_NAND_PIN_NC) {
        route_pin((uint8_t)pin_hd);
    }
    if (wp_pin != STUB_LIB_NAND_PIN_NC) {
        route_pin((uint8_t)wp_pin);
    }

    esp_rom_gpio_connect_out_signal((uint8_t)pin_clk, SPI2_CK_PAD_OUT_IDX, false, false);
    esp_rom_gpio_connect_out_signal((uint8_t)pin_d, SPI2_D_PAD_OUT_IDX, false, false);
    esp_rom_gpio_connect_out_signal((uint8_t)pin_cs, SPI2_CS_PAD_OUT_IDX, false, false);
    esp_rom_gpio_connect_in_signal((uint8_t)pin_q, SPI2_Q_PAD_IN_IDX, false);
    if (pin_hd != STUB_LIB_NAND_PIN_NC) {
        esp_rom_gpio_connect_out_signal((uint8_t)pin_hd, SPI2_HOLD_PAD_OUT_IDX, false, false);
        esp_rom_gpio_output_enable((uint8_t)pin_hd);
    }
    if (wp_pin != STUB_LIB_NAND_PIN_NC) {
        esp_rom_gpio_set_output_level((uint8_t)wp_pin, 1);
        esp_rom_gpio_output_enable((uint8_t)wp_pin);
    }

    esp_rom_gpio_output_enable((uint8_t)pin_clk);
    esp_rom_gpio_output_enable((uint8_t)pin_d);
    esp_rom_gpio_output_enable((uint8_t)pin_cs);
    esp_rom_gpio_output_disable((uint8_t)pin_q);

    /* XTAL 40 MHz / 2 = 20 MHz. */
    REG_WRITE(SPI_CLOCK_REG(NAND_SPI_NUM), (1U << SPI_CLKCNT_N_S) | (1U << SPI_CLKCNT_L_S));
    REG_WRITE(SPI_MISC_REG(NAND_SPI_NUM), 0);
    REG_WRITE(SPI_USER_REG(NAND_SPI_NUM), SPI_CS_SETUP | SPI_CS_HOLD | SPI_DOUTDIN);
    REG_SET_FIELD(SPI_USER1_REG(NAND_SPI_NUM), SPI_CS_SETUP_TIME, 1U);
    REG_SET_FIELD(SPI_USER1_REG(NAND_SPI_NUM), SPI_CS_HOLD_TIME, 1U);

    return 0;
}
