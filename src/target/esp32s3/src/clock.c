/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <soc_utils.h>

#include <target/clock.h>

#include <soc/rtc_cntl_reg.h>
#include <soc/soc.h>
#include <soc/system_reg.h>

#define CPU_FREQ_MHZ 40

extern uint32_t esp_rom_get_cpu_ticks_per_us(void);
extern void esp_rom_set_cpu_ticks_per_us(uint32_t ticks_per_us);
extern uint32_t esp_rom_get_xtal_freq(void);
extern uint32_t esp_rom_get_apb_frequency(void);

static uint32_t s_cpu_freq = 0;

void stub_target_clock_init(void)
{
    // TODO: Increase DBIAS voltage before clock increase,
    // not that simple for esp32s3 as ESP-IDF reads it from eFuse.
    // Without it there were stability issues.
    // https://github.com/espressif/esptool/issues/832, https://github.com/espressif/esptool/issues/808
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
    uint32_t clock;
    uint32_t clock_sel = REG_GET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_SOC_CLK_SEL);
    if (clock_sel == 0) { // from xtal, 80MHz, 40MHz, 20MHz, 10MHz, 8MHz,...
        // Should be also divided by SYSTEM_PRE_DIV_CNT, but setting divider has no effect on ESP32S3.
        clock = esp_rom_get_xtal_freq();
    } else if (clock_sel == 1) { // from pll, 80MHz
        clock = 80 * MHZ;
    } else if (clock_sel == 2) { // 8M RC, about 8MHz, code will not come here
        clock = 8 * MHZ;
    } else { // audio pll, code will not come here, just put an different clock here.
        clock = 16 * MHZ;
    }
    return clock;
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
