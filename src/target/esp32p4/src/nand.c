/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * ESP32-P4 GPSPI2 bring-up for the NAND plugin. The command sequence lives in
 * nand_proto.c. This file uses the ECO5 ROM GPIO entry points, which is the
 * ROM selected by TARGET_CHIP=esp32p4.
 */

#include <stdbool.h>
#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/nand.h>
#include <target/nand_bus.h>

#include <soc/reg_base.h>
#include <soc/spi_reg.h>

#define NAND_SPI_NUM                        2

/* Same default pin numbers as the ESP32-S3 FSPI bus. P4 has no native
 * function on these pads, so every pin goes through the GPIO matrix. */
#define PIN_MOSI                            11
#define PIN_MISO                            13
#define PIN_CLK                             12
#define PIN_CS                              10
#define PIN_WP                              14
#define PIN_HD                              9

#define NAND_GPIO_NUM                       55

/* GPIO matrix signal indices for GPSPI2. */
#define SPI2_CK_OUT_IDX                     53
#define SPI2_Q_IN_IDX                       54
#define SPI2_D_OUT_IDX                      55
#define SPI2_HD_OUT_IDX                     56
#define SPI2_WP_OUT_IDX                     57
#define SPI2_CS_OUT_IDX                     62

/* HP_SYS_CLKRST fields for GPSPI2. */
#define HP_SYS_CLKRST_SOC_CLK_CTRL1_REG     (DR_REG_HP_SYS_CLKRST_BASE + 0x18)
#define HP_SYS_CLKRST_REG_GPSPI2_SYS_CLK_EN (BIT(0))
#define HP_SYS_CLKRST_SOC_CLK_CTRL2_REG     (DR_REG_HP_SYS_CLKRST_BASE + 0x1c)
#define HP_SYS_CLKRST_REG_GPSPI2_APB_CLK_EN (BIT(19))
#define HP_SYS_CLKRST_HP_RST_EN2_REG        (DR_REG_HP_SYS_CLKRST_BASE + 0xc8)
#define HP_SYS_CLKRST_REG_RST_EN_SPI2       (BIT(7))
#define HP_SYS_CLKRST_PERI_CLK_CTRL116_REG  (DR_REG_HP_SYS_CLKRST_BASE + 0x80)
#define HP_SYS_CLKRST_REG_GPSPI2_HS_CLK_EN  (BIT(3))
#define HP_SYS_CLKRST_REG_GPSPI2_MST_CLK_EN (BIT(20))

/* ECO5 ROM. The unprefixed names belong to the rev1 ROM. */
extern void rom_gpio_pad_select_gpio(uint32_t gpio_num);
extern void rom_gpio_pad_input_enable(uint32_t gpio_num);
extern void rom_gpio_pad_set_drv(uint32_t gpio_num, uint32_t drv);
extern void rom_gpio_output_enable(uint32_t gpio_num);
extern void rom_gpio_output_disable(uint32_t gpio_num);
extern void rom_gpio_matrix_out(uint32_t gpio_num, uint32_t signal_idx, bool out_inv, bool oen_inv);
extern void rom_gpio_matrix_in(uint32_t gpio_num, uint32_t signal_idx, bool inv);

static void route_pin(uint8_t pin)
{
    rom_gpio_pad_select_gpio(pin);
    rom_gpio_pad_input_enable(pin);
    rom_gpio_pad_set_drv(pin, 2);
}

int nand_spi_init(uint32_t hspi_arg)
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

    if (pin_clk >= NAND_GPIO_NUM || pin_q >= NAND_GPIO_NUM || pin_d >= NAND_GPIO_NUM || pin_cs >= NAND_GPIO_NUM ||
        pin_hd >= NAND_GPIO_NUM) {
        return NAND_ERR_PIN_INVALID;
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

    route_pin(pin_clk);
    route_pin(pin_q);
    route_pin(pin_d);
    route_pin(pin_cs);
    route_pin(pin_hd);
    route_pin(PIN_WP);

    rom_gpio_matrix_out(pin_clk, SPI2_CK_OUT_IDX, false, false);
    rom_gpio_matrix_out(pin_d, SPI2_D_OUT_IDX, false, false);
    rom_gpio_matrix_out(pin_cs, SPI2_CS_OUT_IDX, false, false);
    rom_gpio_matrix_in(pin_q, SPI2_Q_IN_IDX, false);
    rom_gpio_matrix_out(pin_hd, SPI2_HD_OUT_IDX, false, false);
    rom_gpio_matrix_out(PIN_WP, SPI2_WP_OUT_IDX, false, false);

    rom_gpio_output_enable(pin_clk);
    rom_gpio_output_enable(pin_d);
    rom_gpio_output_enable(pin_cs);
    rom_gpio_output_enable(pin_hd);
    rom_gpio_output_enable(PIN_WP);
    rom_gpio_output_disable(pin_q);

    /* XTAL 40 MHz / 2 = 20 MHz. */
    REG_WRITE(SPI_CLOCK_REG(NAND_SPI_NUM), (1U << SPI_CLKCNT_N_S) | (1U << SPI_CLKCNT_L_S));
    REG_WRITE(SPI_MISC_REG(NAND_SPI_NUM), 0);
    REG_WRITE(SPI_USER_REG(NAND_SPI_NUM), SPI_CS_SETUP | SPI_CS_HOLD | SPI_DOUTDIN);
    REG_SET_FIELD(SPI_USER1_REG(NAND_SPI_NUM), SPI_CS_SETUP_TIME, 1U);
    REG_SET_FIELD(SPI_USER1_REG(NAND_SPI_NUM), SPI_CS_HOLD_TIME, 1U);

    return 0;
}
