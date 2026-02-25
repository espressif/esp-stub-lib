/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>

#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>

#include <soc/dport_reg.h>

extern void Cache_Flush_rom(int cpu_num);
extern void Cache_Read_Disable_rom(int cpu_num);
extern void Cache_Read_Enable_rom(int cpu_num);

typedef struct {
    bool pro_cache_enabled;
    bool app_cache_enabled;
} esp32_cache_state_t;

static esp32_cache_state_t s_cache_state;

void stub_target_cache_save(const void **state_out)
{
    s_cache_state.pro_cache_enabled = DPORT_REG_GET_BIT(DPORT_PRO_CACHE_CTRL_REG, DPORT_PRO_CACHE_ENABLE);
    s_cache_state.app_cache_enabled = DPORT_REG_GET_BIT(DPORT_APP_CACHE_CTRL_REG, DPORT_APP_CACHE_ENABLE);

    if (s_cache_state.pro_cache_enabled) {
        Cache_Read_Disable_rom(0);
    }

    if (s_cache_state.app_cache_enabled) {
        Cache_Read_Disable_rom(1);
    }

    *state_out = &s_cache_state;
}

void stub_target_cache_restore(const void *state)
{
    const esp32_cache_state_t *s = state;

    if (s->pro_cache_enabled) {
        Cache_Read_Enable_rom(0);
        Cache_Flush_rom(0);
    }

    if (s->app_cache_enabled) {
        Cache_Read_Enable_rom(1);
        Cache_Flush_rom(1);
    }
}
