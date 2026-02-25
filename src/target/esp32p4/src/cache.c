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

extern void ROM_Boot_Cache_Init(void);
extern void Cache_Enable_L2_Cache(uint32_t autoload);
extern uint32_t Cache_Disable_L2_Cache(void);
extern void Cache_Set_L2_Cache_Mode(uint32_t size, uint32_t ways, uint32_t line_size);
extern void Cache_FLASH_MMU_Init(void);

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
    Cache_Invalidate_Addr(CACHE_MAP_L1_DCACHE, vaddr, size);
}

/* CRITICAL: Do NOT call full L1 ICache suspend — stub runs from L1-cached
 * region, full suspend blocks instruction fetch and causes a PMP fault.
 * Use autoload-only suspend for all L1; full suspend only for L2.
 */
static uint32_t s_saved_autoload;

void stub_target_cache_stop(void)
{
    s_saved_autoload = 0;

    if (Cache_Suspend_L1_CORE0_ICache_Autoload())
        s_saved_autoload |= BIT(0);
    if (Cache_Suspend_L1_CORE1_ICache_Autoload())
        s_saved_autoload |= BIT(1);
    if (Cache_Suspend_L1_DCache_Autoload())
        s_saved_autoload |= BIT(2);
    if (Cache_Suspend_L2_Cache())
        s_saved_autoload |= BIT(3);
}

void stub_target_cache_start(void)
{
    Cache_Resume_L2_Cache(s_saved_autoload & BIT(3) ? 1 : 0);
    Cache_Resume_L1_DCache_Autoload(s_saved_autoload & BIT(2) ? 1 : 0);
    Cache_Resume_L1_CORE0_ICache_Autoload(s_saved_autoload & BIT(0) ? 1 : 0);
    Cache_Resume_L1_CORE1_ICache_Autoload(s_saved_autoload & BIT(1) ? 1 : 0);
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
        STUB_LOGD("L2 cache not enabled, initializing\n");
        ROM_Boot_Cache_Init();
        Cache_Enable_L2_Cache(0);
    }
}
