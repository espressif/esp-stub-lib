/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>

#include <soc/cache_reg.h>
#include <soc/spi_mem_compat.h>

extern void Cache_Invalidate_All(void);
extern int Cache_Invalidate_Addr(uint32_t addr, uint32_t size);
extern void Cache_WriteBack_All(void);
extern int Cache_WriteBack_Addr(uint32_t addr, uint32_t size);
void Cache_WriteBack_Invalidate_All(void);
extern uint32_t Cache_Suspend_Cache(void);
extern void Cache_Resume_Cache(uint32_t autoload);

void stub_target_cache_writeback_all(void)
{
    // Cache_WriteBack_All();
    // Cache_WriteBack_Invalidate_All();
}

void stub_target_cache_writeback_addr(uint32_t vaddr, uint32_t size)
{
    Cache_WriteBack_Addr(vaddr, size);
}

void stub_target_cache_invalidate_all(void)
{
    Cache_Invalidate_All();
}

void stub_target_cache_invalidate_addr(uint32_t vaddr, uint32_t size)
{
    Cache_Invalidate_Addr(vaddr, size);
}

/* State packing: BIT(0) = cache autoload */
uint32_t stub_target_cache_suspend(void)
{
    uint32_t state = 0;

    if (Cache_Suspend_Cache())
        state |= BIT(0);

    return state;
}

void stub_target_cache_resume(uint32_t autoload)
{
    Cache_Resume_Cache(autoload & BIT(0) ? 1 : 0);
}
