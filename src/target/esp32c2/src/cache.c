/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>

#include <esp-stub-lib/log.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>

#include <soc/ext_mem_reg.h>

extern void Cache_Suspend_ICache(void);
extern void Cache_Resume_ICache(uint32_t autoload);
extern void Cache_Invalidate_ICache_All(void);

typedef struct {
    uint32_t mmu_page_size;
    bool cache_enabled;
} esp32c2_cache_state_t;

static esp32c2_cache_state_t s_cache_state;

void stub_target_cache_save(const void **state_out)
{
    s_cache_state.cache_enabled = REG_GET_BIT(EXTMEM_ICACHE_CTRL_REG, EXTMEM_ICACHE_ENABLE);
    s_cache_state.mmu_page_size = REG_GET_FIELD(EXTMEM_CACHE_CONF_MISC_REG, EXTMEM_CACHE_MMU_PAGE_SIZE);

    STUB_LOGD("MMU page size: 0x%x cache enabled: %d\n", s_cache_state.mmu_page_size, s_cache_state.cache_enabled);

    if (s_cache_state.cache_enabled) {
        Cache_Suspend_ICache();
    }

    *state_out = &s_cache_state;
}

void stub_target_cache_restore(const void *state)
{
    const esp32c2_cache_state_t *s = state;

    /* Restore MMU page size */
    REG_SET_FIELD(EXTMEM_CACHE_CONF_MISC_REG, EXTMEM_CACHE_MMU_PAGE_SIZE, s->mmu_page_size);

    if (s->cache_enabled) {
        /* Invalidate stale lines from flash writes */
        Cache_Invalidate_ICache_All();
        Cache_Resume_ICache(EXTMEM_ICACHE_AUTOLOAD_ENA);
    }
}
