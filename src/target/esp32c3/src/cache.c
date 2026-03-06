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
extern void Cache_Invalidate_ICache_All(void);

typedef struct {
    bool cache_enabled;
    bool autoload_enabled;
} esp32c3_cache_state_t;

static esp32c3_cache_state_t s_cache_state;

void stub_target_cache_save(const void **state_out)
{
    s_cache_state.cache_enabled = REG_GET_BIT(EXTMEM_ICACHE_CTRL_REG, EXTMEM_ICACHE_ENABLE);

    /* On esp32c3, MMU Page size is always 64KB. No need to save it. */

    if (s_cache_state.cache_enabled) {
        s_cache_state.autoload_enabled = Cache_Suspend_ICache();
    }

    *state_out = &s_cache_state;
}

void stub_target_cache_restore(const void *state)
{
    const esp32c3_cache_state_t *s = state;

    if (s->cache_enabled) {
        /* Invalidate stale lines from flash writes */
        Cache_Invalidate_ICache_All();
        Cache_Resume_ICache(s->autoload_enabled ? EXTMEM_ICACHE_AUTOLOAD_ENA : 0U);
    }
}
