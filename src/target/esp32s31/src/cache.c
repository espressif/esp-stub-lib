/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/log.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>
#include <target/mmu.h>

#include <soc/cache_reg.h>

#define CACHE_MAP_L1_ICACHE_0 BIT(0)
#define CACHE_MAP_L1_ICACHE_1 BIT(1)
#define CACHE_MAP_L1_DCACHE   BIT(4)
#define CACHE_MAP_L1_ICACHE   (CACHE_MAP_L1_ICACHE_0 | CACHE_MAP_L1_ICACHE_1)
#define CACHE_MAP_ALL         (CACHE_MAP_L1_ICACHE | CACHE_MAP_L1_DCACHE)

extern int Cache_Invalidate_All(uint32_t map);
extern int Cache_Invalidate_Addr(uint32_t map, uint32_t addr, uint32_t size);
extern void Cache_WriteBack_All(uint32_t map);
extern int Cache_WriteBack_Addr(uint32_t map, uint32_t addr, uint32_t size);

extern uint32_t Cache_Suspend_L1_CORE0_ICache(void);
extern uint32_t Cache_Suspend_L1_CORE1_ICache(void);
extern uint32_t Cache_Suspend_L1_DCache(void);
extern void Cache_Resume_L1_CORE0_ICache(uint32_t autoload);
extern void Cache_Resume_L1_CORE1_ICache(uint32_t autoload);
extern void Cache_Resume_L1_DCache(uint32_t autoload);

extern void ROM_Boot_Cache_Init(void);
extern uint32_t Cache_Disable_L1_DCache(void);

/* Suspend saves three independent opaque autoload values that must be passed
 * back as-is to their matching Resume functions. Since stub_target_cache_suspend
 * returns only a single uint32_t, we keep the three values in a static area and
 * use the return value purely as a "suspended" flag (non-zero = suspended). */
static struct {
    uint32_t core0_icache;
    uint32_t core1_icache;
    uint32_t dcache;
} s_suspend_state;

uint32_t stub_target_cache_get_caps(void)
{
    return STUB_CACHE_CAP_HAS_INVALIDATE_ADDR | STUB_CACHE_CAP_SHARED_IDCACHE;
}

void stub_target_cache_writeback_addr(uint32_t vaddr, uint32_t size)
{
    Cache_WriteBack_Addr(CACHE_MAP_L1_DCACHE, vaddr, size);
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
    Cache_Invalidate_Addr(CACHE_MAP_L1_ICACHE, vaddr, size);
    Cache_Invalidate_Addr(CACHE_MAP_L1_DCACHE, vaddr, size);
}

/* State packing:
 * BIT(0) = CORE0 ICache autoload was enabled
 * BIT(1) = CORE1 ICache autoload was enabled
 * BIT(2) = DCache autoload was enabled
 */
uint32_t stub_target_cache_suspend(void)
{
    s_suspend_state.core0_icache = Cache_Suspend_L1_CORE0_ICache();
    s_suspend_state.core1_icache = Cache_Suspend_L1_CORE1_ICache();
    s_suspend_state.dcache = Cache_Suspend_L1_DCache();
    return 1;
}

void stub_target_cache_resume(uint32_t autoload)
{
    if (!autoload)
        return;
    Cache_Resume_L1_CORE0_ICache(s_suspend_state.core0_icache);
    Cache_Resume_L1_CORE1_ICache(s_suspend_state.core1_icache);
    Cache_Resume_L1_DCache(s_suspend_state.dcache);
}

int stub_target_cache_is_enabled(void)
{
    uint32_t icache_ctrl = REG_READ(CACHE_L1_ICACHE_CTRL_REG);
    if (icache_ctrl & (CACHE_L1_ICACHE_SHUT_IBUS0 | CACHE_L1_ICACHE_SHUT_IBUS1))
        return 0;

    uint32_t dcache_ctrl = REG_READ(CACHE_L1_DCACHE_CTRL_REG);
    if (dcache_ctrl & (CACHE_L1_DCACHE_SHUT_DBUS0 | CACHE_L1_DCACHE_SHUT_DBUS1))
        return 0;

    return stub_target_mmu_has_valid_entry();
}

void stub_target_cache_init(void *state)
{
    (void)state;

    bool cache_was_enabled = stub_target_cache_is_enabled();
    if (!cache_was_enabled) {
        STUB_LOGD("DCache not enabled, initializing for DROM\n");
        ROM_Boot_Cache_Init();
    }
}
