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

#include <soc/ext_mem_reg.h>

extern void Cache_Suspend_ICache(void);
extern void Cache_Resume_ICache(uint32_t autoload);
extern void Cache_Invalidate_ICache_All(void);
extern int Cache_Invalidate_Addr(uint32_t addr, uint32_t size);
extern void ROM_Boot_Cache_Init(void);
extern void Cache_Disable_ICache(void);

typedef struct {
    uint32_t mmu_page_size;
    uint32_t ctrl1;
    bool cache_was_enabled;
} esp32c2_cache_state_t;

size_t stub_target_cache_state_size(void)
{
    return sizeof(esp32c2_cache_state_t);
}

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

/* State packing: BIT(0) = icache enabled */
uint32_t stub_target_cache_suspend(void)
{
    uint32_t state = 0;

    if (REG_GET_BIT(EXTMEM_ICACHE_CTRL_REG, EXTMEM_ICACHE_ENABLE)) {
        state |= BIT(0);
        Cache_Suspend_ICache();
    }

    return state;
}

void stub_target_cache_resume(uint32_t autoload)
{
    if (autoload & BIT(0)) {
        Cache_Resume_ICache(EXTMEM_ICACHE_AUTOLOAD_ENA);
    }
}

int stub_target_cache_is_enabled(void)
{
    uint32_t ctrl1 = REG_READ(EXTMEM_ICACHE_CTRL1_REG);
    return REG_GET_BIT(EXTMEM_ICACHE_CTRL_REG, EXTMEM_ICACHE_ENABLE) && !(ctrl1 & EXTMEM_ICACHE_SHUT_DBUS);
}

void stub_target_cache_init(void *state)
{
    bool cache_was_enabled = stub_target_cache_is_enabled();

    if (state) {
        esp32c2_cache_state_t *s = state;
        s->mmu_page_size = REG_GET_FIELD(EXTMEM_CACHE_CONF_MISC_REG, EXTMEM_CACHE_MMU_PAGE_SIZE);
        s->ctrl1 = REG_READ(EXTMEM_ICACHE_CTRL1_REG);
        s->cache_was_enabled = cache_was_enabled;
        STUB_LOGD("mmu_page_size: %d cache_ctrl: 0x%x\n", s->mmu_page_size, s->ctrl1);
    }

    if (!cache_was_enabled) {
        STUB_LOGD("ICache not enabled, initializing for DROM\n");
        ROM_Boot_Cache_Init();
    }
}

void stub_target_cache_deinit(const void *state)
{
    if (!state)
        return;

    const esp32c2_cache_state_t *s = state;

    if (!s->cache_was_enabled) {
        STUB_LOGD("Disabling ICache\n");
        Cache_Disable_ICache();
        REG_WRITE(EXTMEM_ICACHE_CTRL1_REG, s->ctrl1);
    } else {
        /* Restore MMU page size */
        REG_SET_FIELD(EXTMEM_CACHE_CONF_MISC_REG, EXTMEM_CACHE_MMU_PAGE_SIZE, s->mmu_page_size);
    }
}
