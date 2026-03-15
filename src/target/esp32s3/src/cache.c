/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>

#include <soc/ext_mem_reg.h>

extern uint32_t Cache_Suspend_ICache(void);
extern uint32_t Cache_Suspend_DCache(void);
extern void Cache_Resume_ICache(uint32_t autoload);
extern void Cache_Resume_DCache(uint32_t autoload);
extern void Cache_Invalidate_ICache_All(void);
extern void Cache_Invalidate_DCache_All(void);
extern int Cache_Invalidate_Addr(uint32_t addr, uint32_t size);
extern void Cache_WriteBack_All(void);
extern int Cache_WriteBack_Addr(uint32_t addr, uint32_t size);

void stub_target_cache_writeback_all(void)
{
    // Cache_WriteBack_All();
}

void stub_target_cache_writeback_addr(uint32_t vaddr, uint32_t size)
{
    Cache_WriteBack_Addr(vaddr, size);
}

void stub_target_cache_invalidate_all(void)
{
    Cache_Invalidate_DCache_All();
    Cache_Invalidate_ICache_All();
}

void stub_target_cache_invalidate_addr(uint32_t vaddr, uint32_t size)
{
    Cache_Invalidate_Addr(vaddr, size);
}

/* State packing: BIT(0) = icache enabled, BIT(1) = dcache enabled,
 *                BIT(2) = icache autoload, BIT(3) = dcache autoload */
uint32_t stub_target_cache_suspend(void)
{
    uint32_t state = 0;

    if (REG_GET_BIT(EXTMEM_ICACHE_CTRL_REG, EXTMEM_ICACHE_ENABLE)) {
        state |= BIT(0);
        if (Cache_Suspend_ICache())
            state |= BIT(2);
    }

    if (REG_GET_BIT(EXTMEM_DCACHE_CTRL_REG, EXTMEM_DCACHE_ENABLE)) {
        state |= BIT(1);
        if (Cache_Suspend_DCache())
            state |= BIT(3);
    }

    return state;
}

void stub_target_cache_resume(uint32_t autoload)
{
    if (autoload & BIT(1)) {
        Cache_Resume_DCache(autoload & BIT(3) ? EXTMEM_DCACHE_AUTOLOAD_ENA : 0);
    }

    if (autoload & BIT(0)) {
        Cache_Resume_ICache(autoload & BIT(2) ? EXTMEM_ICACHE_AUTOLOAD_ENA : 0);
    }
}
