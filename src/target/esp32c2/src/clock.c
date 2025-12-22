/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <target/clock.h>
#include <soc_utils.h>
#include <soc/system_reg.h>
#include <soc/uart_reg.h>
#include <soc/soc.h>

extern uint32_t esp_rom_get_xtal_freq(void);
extern void esp_rom_set_cpu_ticks_per_us(uint32_t ticks_per_us);
extern uint32_t esp_rom_get_cpu_ticks_per_us(void);
extern uint32_t esp_rom_get_apb_freq(void);

static uint32_t s_cpu_freq = 0;
static uint32_t s_crystal_freq = 0;

void stub_target_clock_init(void)
{
    // This enables PLL which multiplies the crystal frequency by 12/6 = 2x.
    REG_SET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_SOC_CLK_SEL, 1);
    REG_SET_FIELD(SYSTEM_CPU_PER_CONF_REG, SYSTEM_PLL_FREQ_SEL, 0);

    /*
    ESP32-C2 does not have ROM code to detect the crystal frequency,
    so we use the previously auto-synced 115200 baud rate which
    is set by previous init code to estimate the crystal frequency.
    TODO: Improve this to not depend on the previous init code.
    TODO: Increase CPU frequency to at least 80 MHz for 26 MHz crystal.
    */
    uint32_t uart_div_int = REG_GET_FIELD(UART_CLKDIV_REG(0), UART_CLKDIV);
    uint32_t uart_div_frag = REG_GET_FIELD(UART_CLKDIV_REG(0), UART_CLKDIV_FRAG);
    uint32_t uart_div = (uart_div_int << 4) + uart_div_frag;
    uint32_t approx_crystal_freq = (uart_div * 115200) >> 4;

    s_crystal_freq = (approx_crystal_freq > (33 * MHZ)) ? (40 * MHZ) : (26 * MHZ);
    s_cpu_freq = s_crystal_freq * 2;
    esp_rom_set_cpu_ticks_per_us(s_cpu_freq / MHZ);
}

uint32_t stub_target_get_cpu_freq(void)
{
    if (s_cpu_freq == 0) {
        return esp_rom_get_cpu_ticks_per_us() * MHZ;
    }
    return s_cpu_freq;
}

uint32_t stub_target_get_apb_freq(void)
{
    return s_crystal_freq;
}
