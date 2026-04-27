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
#include <soc/spi_mem_compat.h>

extern void ROM_Boot_Cache_Init(void);
extern uint32_t Cache_Disable_ICache(void);
extern uint32_t Cache_Suspend_ICache(void);
extern void Cache_Resume_ICache(uint32_t autoload);
extern void Cache_Invalidate_ICache_All(void);
extern int Cache_Invalidate_Addr(uint32_t addr, uint32_t size);

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

/* State packing: BIT(0) = icache autoload */
uint32_t stub_target_cache_suspend(void)
{
    uint32_t state = 0;

    if (Cache_Suspend_ICache())
        state |= BIT(0);

    return state;
}

void stub_target_cache_resume(uint32_t autoload)
{
    Cache_Resume_ICache(autoload & BIT(0) ? CACHE_L1_CACHE_AUTOLOAD_ENA : 0);
}

int stub_target_cache_is_enabled(void)
{
    uint32_t ctrl = REG_READ(CACHE_L1_CACHE_CTRL_REG);
    return stub_target_mmu_has_valid_entry() && !(ctrl & (CACHE_L1_CACHE_SHUT_BUS0 | CACHE_L1_CACHE_SHUT_BUS1));
}

void stub_target_cache_init(void *state)
{
    (void)state;

    bool cache_was_enabled = stub_target_cache_is_enabled();
    if (!cache_was_enabled) {
        STUB_LOGD("ICache not enabled, initializing for DROM\n");
        ROM_Boot_Cache_Init();
    }
}
