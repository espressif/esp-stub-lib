/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>

#include <soc/ext_mem_reg.h>

extern void Cache_Suspend_ICache(void);
extern void Cache_Resume_ICache(uint32_t autoload);
extern void Cache_Invalidate_ICache_All(void);
extern int Cache_Invalidate_Addr(uint32_t addr, uint32_t size);

typedef struct {
    uint32_t mmu_page_size;
} esp32c2_cache_state_t;

static esp32c2_cache_state_t s_cache_state;

uint32_t stub_target_cache_get_caps(void)
{
    return STUB_CACHE_CAP_HAS_INVALIDATE_ADDR | STUB_CACHE_CAP_SHARED_IDCACHE;
}

void stub_target_cache_invalidate_all(void)
{
    Cache_Invalidate_ICache_All();
}

void stub_target_cache_invalidate_addr(uint32_t vaddr, uint32_t size)
{
    Cache_Invalidate_Addr(vaddr, size);
}

/* State packing: BIT(0) = icache enabled */
uint32_t stub_target_cache_suspend(void)
{
    uint32_t state = 0;

    if (REG_GET_BIT(EXTMEM_ICACHE_CTRL_REG, EXTMEM_ICACHE_ENABLE)) {
        state |= BIT(0);
        Cache_Suspend_ICache();
    }

    return state;
}

void stub_target_cache_resume(uint32_t autoload)
{
    if (autoload & BIT(0)) {
        Cache_Resume_ICache(EXTMEM_ICACHE_AUTOLOAD_ENA);
    }
}

void stub_target_cache_save(void)
{
    s_cache_state.mmu_page_size = REG_GET_FIELD(EXTMEM_CACHE_CONF_MISC_REG, EXTMEM_CACHE_MMU_PAGE_SIZE);
}

void stub_target_cache_restore(void)
{
    /* Restore MMU page size */
    REG_SET_FIELD(EXTMEM_CACHE_CONF_MISC_REG, EXTMEM_CACHE_MMU_PAGE_SIZE, s_cache_state.mmu_page_size);
}
