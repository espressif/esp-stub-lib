/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <target/clock.h>
#include <soc_utils.h>
#include <soc/system_reg.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/soc.h>

#define CPU_FREQ_MHZ 240

extern void esp_rom_set_cpu_ticks_per_us(uint32_t ticks_per_us);
extern uint32_t esp_rom_get_cpu_ticks_per_us(void);
extern uint32_t esp_rom_get_apb_frequency(void);

static uint32_t s_cpu_freq = 0;

void stub_target_clock_init(void)
{

    REG_SET_FIELD(RTC_CNTL_REG, RTC_CNTL_DIG_DBIAS_WAK, RTC_CNTL_DIG_DBIAS_1V20);
    REG_SET_FIELD(RTC_CNTL_REG, RTC_CNTL_DBIAS_WAK, RTC_CNTL_DIG_DBIAS_1V20);

    REG_SET_FIELD(DPORT_SYSCLK_CONF_REG, DPORT_SOC_CLK_SEL, 1);
    REG_SET_FIELD(DPORT_CPU_PER_CONF_REG, DPORT_PLL_FREQ_SEL, 1);
    REG_SET_FIELD(DPORT_CPU_PER_CONF_REG, DPORT_CPUPERIOD_SEL, 2);
    s_cpu_freq = CPU_FREQ_MHZ * MHZ;
    esp_rom_set_cpu_ticks_per_us(CPU_FREQ_MHZ);
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
    return esp_rom_get_apb_frequency();
}

#define RTC_CNTL_WDT_KEY 0x50D83AA1
#define RTC_CNTL_SWD_KEY 0x8F1D312A

void stub_target_clock_disable_watchdogs(void)
{
    // Disable RWDT (RTC Watchdog)
    REG_SET_BIT(RTC_CNTL_INT_CLR_REG, RTC_CNTL_WDT_INT_CLR);
    WRITE_PERI_REG(RTC_CNTL_WDTWPROTECT_REG, RTC_CNTL_WDT_KEY);
    WRITE_PERI_REG(RTC_CNTL_WDTCONFIG0_REG, 0x0);
    WRITE_PERI_REG(RTC_CNTL_WDTWPROTECT_REG, 0x0);

    // Configure SWD (Super Watchdog) to autofeed
    REG_SET_BIT(RTC_CNTL_INT_CLR_REG, RTC_CNTL_SWD_INT_CLR);
    WRITE_PERI_REG(RTC_CNTL_SWD_WPROTECT_REG, RTC_CNTL_SWD_KEY);
    SET_PERI_REG_MASK(RTC_CNTL_SWD_CONF_REG, RTC_CNTL_SWD_AUTO_FEED_EN);
    WRITE_PERI_REG(RTC_CNTL_SWD_WPROTECT_REG, 0x0);
}
