/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <soc_utils.h>

#include <target/clock.h>

#include <soc/lp_wdt_reg.h>
#include <soc/pcr_reg.h>
#include <soc/soc.h>

/* CPU clock sources: 0 = XTAL (32 MHz), 1 = SPLL, 2 = FOSC, 3 = XTAL_X2. */
#define SOC_CPU_CLK_SRC_PLL 1U

/* 96 MHz is the highest frequency ESP32-H21 supports (SPLL, CPU divider of 1). */
#define CPU_FREQ_MHZ        96

/*
 * AHB_CLK is derived from CPU_CLK and must not exceed 32 MHz, so the CPU
 * divider of 1 pairs with an AHB divider of 3.
 */
#define CPU_DIV             1U
#define AHB_DIV             3U

extern uint32_t esp_rom_get_cpu_freq(void);
extern void esp_rom_set_cpu_ticks_per_us(uint32_t ticks_per_us);

static uint32_t s_cpu_freq = 0;

static void update_bus_clocks(void)
{
    REG_SET_FIELD(PCR_BUS_CLK_UPDATE_REG, PCR_BUS_CLOCK_UPDATE, 1);
    while (REG_GET_BIT(PCR_BUS_CLK_UPDATE_REG, PCR_BUS_CLOCK_UPDATE)) {
    }
}

void stub_target_clock_init(void)
{
    s_cpu_freq = CPU_FREQ_MHZ * MHZ;
    esp_rom_set_cpu_ticks_per_us(CPU_FREQ_MHZ);

    /* Program the dividers before switching the root mux. */
    REG_SET_FIELD(PCR_CPU_FREQ_CONF_REG, PCR_CPU_DIV_NUM, CPU_DIV - 1U);
    REG_SET_FIELD(PCR_AHB_FREQ_CONF_REG, PCR_AHB_DIV_NUM, AHB_DIV - 1U);
    REG_SET_FIELD(PCR_SYSCLK_CONF_REG, PCR_SOC_CLK_SEL, SOC_CPU_CLK_SRC_PLL);
    update_bus_clocks();
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
