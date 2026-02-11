/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <soc_utils.h>

#include <target/clock.h>

#include <soc/soc.h>

extern uint32_t esp_rom_get_cpu_ticks_per_us(void);
extern void esp_rom_set_cpu_ticks_per_us(uint32_t ticks_per_us);

void stub_target_clock_init(void)
{
    esp_rom_set_cpu_ticks_per_us(52);
}

void stub_target_clock_disable_watchdogs(void)
{
#define WDT_CTL_REG 0x60000900
    REG_SET_BIT(WDT_CTL_REG, 0);
}
