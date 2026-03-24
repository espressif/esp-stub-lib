/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/log.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>

#include <soc/ext_mem_reg.h>

extern uint32_t Cache_Suspend_ICache(void);
extern void Cache_Resume_ICache(uint32_t autoload);
extern uint32_t Cache_Suspend_DCache(void);
extern void Cache_Resume_DCache(uint32_t autoload);
extern void Cache_Invalidate_ICache_All(void);
extern void Cache_Invalidate_DCache_All(void);
extern void Cache_WriteBack_All(void);
extern int Cache_Invalidate_Addr(uint32_t addr, uint32_t size);
extern int Cache_WriteBack_Addr(uint32_t addr, uint32_t size);
extern void Cache_Allocate_SRAM(int icache_low, int icache_high, int dcache_low, int dcache_high);
extern void Cache_Set_ICache_Mode(int cache_size, int ways, int line_size);
extern void Cache_MMU_Init(void);

#define CACHE_MEMORY_ICACHE_LOW  BIT(0)
#define CACHE_MEMORY_ICACHE_HIGH BIT(1)
#define CACHE_MEMORY_INVALID     0
#define CACHE_SIZE_8KB           0 /* CACHE_SIZE_HALF */
#define CACHE_SIZE_16KB          1 /* CACHE_SIZE_FULL */
#define CACHE_4WAYS_ASSOC        0
#define CACHE_LINE_SIZE_32B      1

static struct {
    uint32_t ctrl1;
    bool cache_was_enabled;
} s_cache_state;

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

    if (REG_GET_BIT(EXTMEM_PRO_ICACHE_CTRL_REG, EXTMEM_PRO_ICACHE_ENABLE)) {
        state |= BIT(0);
        if (Cache_Suspend_ICache())
            state |= BIT(2);
    }

    if (REG_GET_BIT(EXTMEM_PRO_DCACHE_CTRL_REG, EXTMEM_PRO_DCACHE_ENABLE)) {
        state |= BIT(1);
        if (Cache_Suspend_DCache())
            state |= BIT(3);
    }

    return state;
}

void stub_target_cache_resume(uint32_t autoload)
{
    if (autoload & BIT(1)) {
        Cache_Resume_DCache(autoload & BIT(3) ? EXTMEM_PRO_DCACHE_AUTOLOAD_ENA : 0);
    }

    if (autoload & BIT(0)) {
        Cache_Resume_ICache(autoload & BIT(2) ? EXTMEM_PRO_ICACHE_AUTOLOAD_ENA : 0);
    }
}

void stub_target_cache_save(void)
{
    s_cache_state.ctrl1 = REG_READ(EXTMEM_PRO_ICACHE_CTRL1_REG);
    s_cache_state.cache_was_enabled = REG_GET_BIT(EXTMEM_PRO_ICACHE_CTRL_REG, EXTMEM_PRO_ICACHE_ENABLE) &&
                                      !(s_cache_state.ctrl1 & EXTMEM_PRO_ICACHE_MASK_DROM0);

    if (!s_cache_state.cache_was_enabled) {
        STUB_LOGD("ICache not enabled, initializing for DROM0\n");
        Cache_Allocate_SRAM(CACHE_MEMORY_ICACHE_LOW, CACHE_MEMORY_INVALID, CACHE_MEMORY_INVALID, CACHE_MEMORY_INVALID);
        Cache_Suspend_ICache();
        Cache_Set_ICache_Mode(CACHE_SIZE_8KB, CACHE_4WAYS_ASSOC, CACHE_LINE_SIZE_32B);
        Cache_Invalidate_ICache_All();
        Cache_MMU_Init();
        REG_CLR_BIT(EXTMEM_PRO_ICACHE_CTRL1_REG, EXTMEM_PRO_ICACHE_MASK_DROM0);
        Cache_Resume_ICache(0);
    }
}

void stub_target_cache_restore(void)
{
    if (!s_cache_state.cache_was_enabled) {
        STUB_LOGD("Disabling ICache\n");
        Cache_Suspend_ICache();
        REG_WRITE(EXTMEM_PRO_ICACHE_CTRL1_REG, s_cache_state.ctrl1);
    }
}
