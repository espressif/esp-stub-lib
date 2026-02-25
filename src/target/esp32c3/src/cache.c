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

extern uint32_t Cache_Suspend_ICache(void);
extern void Cache_Resume_ICache(uint32_t autoload);
extern void Cache_Invalidate_ICache_All(void);
extern int Cache_Invalidate_Addr(uint32_t addr, uint32_t size);

void stub_target_cache_invalidate_all(void)
{
    Cache_Invalidate_ICache_All();
}

void stub_target_cache_invalidate_addr(uint32_t vaddr, uint32_t size)
{
    Cache_Invalidate_Addr(vaddr, size);
}

/* State packing: BIT(0) = icache enabled, BIT(1) = icache autoload */
uint32_t stub_target_cache_suspend(void)
{
    uint32_t state = 0;

    if (REG_GET_BIT(EXTMEM_ICACHE_CTRL_REG, EXTMEM_ICACHE_ENABLE)) {
        state |= BIT(0);
        if (Cache_Suspend_ICache())
            state |= BIT(1);
    }

    return state;
}

void stub_target_cache_resume(uint32_t autoload)
{
    if (autoload & BIT(0)) {
        Cache_Resume_ICache(autoload & BIT(1) ? EXTMEM_ICACHE_AUTOLOAD_ENA : 0);
    }
}
