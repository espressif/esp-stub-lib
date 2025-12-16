/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <target/clock.h>
#include <soc_utils.h>
#include <soc/system_reg.h>
#include <soc/soc.h>

#define CPU_FREQ_MHZ 40

extern uint32_t esp_rom_get_cpu_ticks_per_us(void);
extern void esp_rom_set_cpu_ticks_per_us(uint32_t ticks_per_us);
extern uint32_t esp_rom_get_xtal_freq(void);

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
    if (REG_GET_FIELD(SYSTEM_CPU_PER_CONF_REG, SYSTEM_PLL_FREQ_SEL) == 1) {
        return 80U * MHZ;
    }
    return esp_rom_get_xtal_freq();
}
