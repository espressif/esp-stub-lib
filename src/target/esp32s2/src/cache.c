/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>

#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>

#include <soc/ext_mem_reg.h>

extern uint32_t Cache_Suspend_ICache(void);
extern void Cache_Resume_ICache(uint32_t autoload);
extern uint32_t Cache_Suspend_DCache(void);
extern void Cache_Resume_DCache(uint32_t autoload);
extern void Cache_Invalidate_ICache_All(void);
extern void Cache_Invalidate_DCache_All(void);
extern void Cache_WriteBack_All(void);

typedef struct {
    bool icache_enabled;
    bool dcache_enabled;
    uint32_t icache_autoload;
    uint32_t dcache_autoload;
} esp32s2_cache_state_t;

static esp32s2_cache_state_t s_cache_state;

void stub_target_cache_save(const void **state_out)
{
    s_cache_state.icache_enabled = REG_GET_BIT(EXTMEM_PRO_ICACHE_CTRL_REG, EXTMEM_PRO_ICACHE_ENABLE);
    s_cache_state.dcache_enabled = REG_GET_BIT(EXTMEM_PRO_DCACHE_CTRL_REG, EXTMEM_PRO_DCACHE_ENABLE);

    if (s_cache_state.icache_enabled) {
        s_cache_state.icache_autoload = Cache_Suspend_ICache();
    }

    if (s_cache_state.dcache_enabled) {
        Cache_WriteBack_All();
        s_cache_state.dcache_autoload = Cache_Suspend_DCache();
    }

    *state_out = &s_cache_state;
}

void stub_target_cache_restore(const void *state)
{
    const esp32s2_cache_state_t *s = state;

    if (s->icache_enabled) {
        Cache_Invalidate_ICache_All();
        Cache_Resume_ICache(s->icache_autoload);
    }

    if (s->dcache_enabled) {
        Cache_Invalidate_DCache_All();
        Cache_Resume_DCache(s->dcache_autoload);
    }
}
