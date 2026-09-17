/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stddef.h>
#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/log.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>
#include <target/mmu.h>

#include <soc/cache_reg.h>
#include <soc/lp_aon_reg.h>
#include <soc/spi_mem_compat.h>

extern void Cache_Freeze_Enable(uint32_t map, int mode);
extern void Cache_Freeze_Disable(uint32_t map);
extern uint32_t Cache_Suspend_Cache(uint32_t map);
extern void Cache_Resume_Cache(uint32_t map, uint32_t autoload);
extern void Cache_Invalidate_All(uint32_t map);
extern int Cache_Invalidate_Addr(uint32_t map, uint32_t addr, uint32_t size);
extern void Cache_WriteBack_All(void);
extern int Cache_WriteBack_Addr(uint32_t addr, uint32_t size);
extern void ROM_Boot_Cache_Init(void);

#define CACHE_MAP_ICACHE0     BIT(0)
#define CACHE_MAP_ICACHE1     BIT(1)
#define CACHE_MAP_DCACHE      BIT(4)
#define CACHE_MAP_ALL         (CACHE_MAP_ICACHE0 | CACHE_MAP_ICACHE1 | CACHE_MAP_DCACHE)
#define CACHE_FREEZE_ACK_BUSY 0

static uint32_t s_saved_autoload;

uint32_t stub_target_cache_get_caps(void)
{
    return STUB_CACHE_CAP_HAS_INVALIDATE_ADDR;
}

void stub_target_cache_writeback_all(void)
{
    Cache_WriteBack_All();
}

void stub_target_cache_writeback_addr(uint32_t vaddr, uint32_t size)
{
    Cache_WriteBack_Addr(vaddr, size);
}

void stub_target_cache_invalidate_all(void)
{
    Cache_Invalidate_All(CACHE_MAP_ALL);
}

void stub_target_cache_invalidate_addr(uint32_t vaddr, uint32_t size)
{
    Cache_Invalidate_Addr(CACHE_MAP_ALL, vaddr, size);
}

void stub_target_cache_stop(void)
{
    s_saved_autoload = Cache_Suspend_Cache(CACHE_MAP_ALL);
}

void stub_target_cache_start(void)
{
    Cache_Resume_Cache(CACHE_MAP_ALL, s_saved_autoload);
}

void stub_target_cache_freeze(void)
{
    Cache_Freeze_Enable(CACHE_MAP_ALL, CACHE_FREEZE_ACK_BUSY);
}

void stub_target_cache_unfreeze(void)
{
    Cache_Freeze_Disable(CACHE_MAP_ALL);
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
        STUB_LOGD("Cache not enabled, initializing for DROM\n");
        ROM_Boot_Cache_Init();
        // TODO: check below is correct
        REG_SET_BIT(LP_AON_SRAM_USAGE_CONF_REG, LP_AON_DCACHE_USAGE);
        REG_SET_BIT(LP_AON_SRAM_USAGE_CONF_REG, LP_AON_ICACHE1_USAGE);
    }
}
