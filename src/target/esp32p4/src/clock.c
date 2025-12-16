/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <target/clock.h>
#include <soc_utils.h>
#include <soc/lp_clkrst_reg.h>
#include <soc/soc.h>
#include <soc/lp_wdt_reg.h>

#define CPU_FREQ_MHZ 320

extern uint32_t esp_rom_get_cpu_freq(void);
extern void esp_rom_set_cpu_ticks_per_us(uint32_t ticks_per_us);

static uint32_t s_cpu_freq = 0;

void stub_target_clock_init(void)
{
    REG_SET_FIELD(LP_CLKRST_HP_CLK_CTRL_REG, LP_CLKRST_HP_ROOT_CLK_SRC_SEL, 1);
    s_cpu_freq = CPU_FREQ_MHZ * MHZ;
    esp_rom_set_cpu_ticks_per_us(CPU_FREQ_MHZ);
}

uint32_t stub_target_get_cpu_freq(void)
{
    if (s_cpu_freq == 0) {
        return esp_rom_get_cpu_freq();
    }
    return s_cpu_freq;
}

#define LP_WDT_WDT_KEY 0x50D83AA1
#define LP_WDT_SWD_KEY 0x50D83AA1

void stub_target_clock_disable_watchdogs(void)
{
    // Disable RWDT (RTC Watchdog)
    REG_SET_BIT(LP_WDT_INT_CLR_REG, LP_WDT_LP_WDT_INT_CLR);
    WRITE_PERI_REG(LP_WDT_WPROTECT_REG, LP_WDT_WDT_KEY);
    WRITE_PERI_REG(LP_WDT_CONFIG0_REG, 0x0);
    WRITE_PERI_REG(LP_WDT_WPROTECT_REG, 0x0);

    // Configure SWD (Super Watchdog) to autofeed
    REG_SET_BIT(LP_WDT_INT_CLR_REG, LP_WDT_SUPER_WDT_INT_CLR);
    WRITE_PERI_REG(LP_WDT_SWD_WPROTECT_REG, LP_WDT_SWD_KEY);
    SET_PERI_REG_MASK(LP_WDT_SWD_CONFIG_REG, LP_WDT_SWD_AUTO_FEED_EN);
    WRITE_PERI_REG(LP_WDT_SWD_WPROTECT_REG, 0x0);
}
