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

#include <soc/ext_mem_reg.h>

extern void ROM_Boot_Cache_Init(void);
extern void Cache_Disable_ICache(void);
extern void Cache_Freeze_ICache_Enable(int mode);
extern void Cache_Freeze_ICache_Disable(void);
extern void Cache_Invalidate_ICache_All(void);
extern int Cache_Invalidate_Addr(uint32_t addr, uint32_t size);

#define CACHE_FREEZE_ACK_BUSY 0

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

void stub_target_cache_stop(void)
{
    Cache_Freeze_ICache_Enable(CACHE_FREEZE_ACK_BUSY);
}

void stub_target_cache_start(void)
{
    Cache_Freeze_ICache_Disable();
}

int stub_target_cache_is_enabled(void)
{
    uint32_t ctrl = REG_READ(EXTMEM_L1_CACHE_CTRL_REG);
    return stub_target_mmu_has_valid_entry() && !(ctrl & (EXTMEM_L1_CACHE_SHUT_DBUS | EXTMEM_L1_CACHE_SHUT_IBUS));
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
