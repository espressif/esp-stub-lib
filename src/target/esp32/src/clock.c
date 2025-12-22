/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <target/clock.h>
#include <soc_utils.h>
#include <soc/reg_base.h>
#include <soc/spi_reg.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/soc.h>

#define CPU_FREQ_MHZ 160

extern void esp_rom_set_cpu_ticks_per_us(uint32_t ticks_per_us);
extern uint32_t esp_rom_get_cpu_ticks_per_us(void);
extern uint32_t esp_rom_get_detected_xtal_freq(void);

static uint32_t s_cpu_freq = 0;

void stub_target_clock_init(void)
{
    REG_SET_FIELD(RTC_CNTL_REG, RTC_CNTL_DIG_DBIAS_WAK, RTC_CNTL_DBIAS_1V25);
    REG_SET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_SOC_CLK_SEL, 1U);
    s_cpu_freq = CPU_FREQ_MHZ * MHZ;
    esp_rom_set_cpu_ticks_per_us(CPU_FREQ_MHZ);
    WRITE_PERI_REG(SPI_CLOCK_REG(0), 0);
}

uint32_t stub_target_get_cpu_freq(void)
{
    if (s_cpu_freq == 0) {
        return esp_rom_get_cpu_ticks_per_us() * MHZ;
    }
    return s_cpu_freq;
}
