/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>

#define CACHE_MAP_L1_ICACHE_0 BIT(0)
#define CACHE_MAP_L1_ICACHE_1 BIT(1)
#define CACHE_MAP_L1_DCACHE   BIT(4)
#define CACHE_MAP_L2_CACHE    BIT(5)
#define CACHE_MAP_L1_ICACHE   (CACHE_MAP_L1_ICACHE_0 | CACHE_MAP_L1_ICACHE_1)
#define CACHE_MAP_ALL         (CACHE_MAP_L1_ICACHE | CACHE_MAP_L1_DCACHE | CACHE_MAP_L2_CACHE)

extern int Cache_Invalidate_All(uint32_t map);
extern int Cache_Invalidate_Addr(uint32_t map, uint32_t addr, uint32_t size);
extern void Cache_WriteBack_All(uint32_t map);
extern int Cache_WriteBack_Addr(uint32_t map, uint32_t addr, uint32_t size);

extern uint32_t Cache_Suspend_L1_CORE0_ICache_Autoload(void);
extern void Cache_Resume_L1_CORE0_ICache_Autoload(uint32_t autoload);
extern uint32_t Cache_Suspend_L1_CORE1_ICache_Autoload(void);
extern void Cache_Resume_L1_CORE1_ICache_Autoload(uint32_t autoload);

extern uint32_t Cache_Suspend_L1_DCache_Autoload(void);
extern void Cache_Resume_L1_DCache_Autoload(uint32_t autoload);
extern uint32_t Cache_Suspend_L2_Cache(void);
extern void Cache_Resume_L2_Cache(uint32_t autoload);

uint32_t stub_target_cache_get_caps(void)
{
    return STUB_CACHE_CAP_HAS_INVALIDATE_ADDR | STUB_CACHE_CAP_SHARED_IDCACHE;
}

void stub_target_cache_writeback_addr(uint32_t vaddr, uint32_t size)
{
    Cache_WriteBack_Addr(CACHE_MAP_L1_DCACHE | CACHE_MAP_L2_CACHE, vaddr, size);
}

void stub_target_cache_writeback_all(void)
{
    Cache_WriteBack_All(CACHE_MAP_ALL);
}

void stub_target_cache_invalidate_all(void)
{
    Cache_Invalidate_All(CACHE_MAP_ALL);
}

void stub_target_cache_invalidate_addr(uint32_t vaddr, uint32_t size)
{
    Cache_Invalidate_Addr(CACHE_MAP_L2_CACHE, vaddr, size);
    Cache_Invalidate_Addr(CACHE_MAP_L1_ICACHE, vaddr, size);
}

/* State packing:
 * BIT(0) = L1 CORE0 ICache autoload
 * BIT(1) = L1 CORE1 ICache autoload
 * BIT(2) = L1 DCache autoload
 * BIT(3) = L2 Cache autoload
 *
 * CRITICAL: Do NOT call full L1 ICache suspend — stub runs from L1-cached
 * region, full suspend blocks instruction fetch and causes a PMP fault.
 * Use autoload-only suspend for all L1; full suspend only for L2.
 */
uint32_t stub_target_cache_suspend(void)
{
    uint32_t state = 0;

    if (Cache_Suspend_L1_CORE0_ICache_Autoload())
        state |= BIT(0);
    if (Cache_Suspend_L1_CORE1_ICache_Autoload())
        state |= BIT(1);
    if (Cache_Suspend_L1_DCache_Autoload())
        state |= BIT(2);
    if (Cache_Suspend_L2_Cache())
        state |= BIT(3);

    return state;
}

void stub_target_cache_resume(uint32_t autoload)
{
    Cache_Resume_L2_Cache(autoload & BIT(3) ? 1 : 0);
    Cache_Resume_L1_DCache_Autoload(autoload & BIT(2) ? 1 : 0);
    Cache_Resume_L1_CORE0_ICache_Autoload(autoload & BIT(0) ? 1 : 0);
    Cache_Resume_L1_CORE1_ICache_Autoload(autoload & BIT(1) ? 1 : 0);
}
