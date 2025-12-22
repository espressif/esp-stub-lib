/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <target/clock.h>
#include <soc_utils.h>
#include <soc/pcr_reg.h>
#include <soc/soc.h>

#define CPU_FREQ_MHZ 96

extern uint32_t esp_rom_get_cpu_freq(void);
extern void esp_rom_set_cpu_ticks_per_us(uint32_t ticks_per_us);

static uint32_t s_cpu_freq = 0;

void stub_target_clock_init(void)
{
    REG_SET_FIELD(PCR_SYSCLK_CONF_REG, PCR_SOC_CLK_SEL, 1);
    REG_SET_FIELD(PCR_BUS_CLK_UPDATE_REG, PCR_BUS_CLOCK_UPDATE, 1);
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
