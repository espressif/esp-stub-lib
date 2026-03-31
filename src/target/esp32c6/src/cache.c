/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/err.h>
#include <esp-stub-lib/log.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>
#include <target/mmu.h>

#include <soc/ext_mem_defs.h>
#include <soc/ext_mem_reg.h>
#include <soc/spi_mem_compat.h>

extern void ROM_Boot_Cache_Init(void);
extern void Cache_Disable_ICache(void);
extern uint32_t Cache_Suspend_ICache(void);
extern void Cache_Resume_ICache(uint32_t autoload);
extern void Cache_Invalidate_ICache_All(void);
extern int Cache_Invalidate_Addr(uint32_t addr, uint32_t size);

typedef struct {
    uint32_t mmu_page_size;
    uint32_t ctrl;
    bool cache_was_enabled;
} esp32c6_cache_state_t;

static esp32c6_cache_state_t s_cache_state;

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
    Cache_Resume_ICache(autoload & BIT(0) ? EXTMEM_L1_CACHE_AUTOLOAD_ENA : 0);
}

static bool is_cache_enabled(void)
{
    /* if any of the entry is valid and buses are enabled we can consider that cache is enabled */
    for (uint32_t i = 0; i < SOC_MMU_ENTRY_NUM; ++i) {
        uint32_t mmu_raw_value = stub_target_mmu_read_entry(i);
        if ((mmu_raw_value & SOC_MMU_VALID) == SOC_MMU_VALID)
            return !(s_cache_state.ctrl & (EXTMEM_L1_CACHE_SHUT_DBUS | EXTMEM_L1_CACHE_SHUT_IBUS));
    }
    return false;
}

void stub_target_cache_save(void)
{
    s_cache_state.mmu_page_size = REG_GET_FIELD(SPI_MEM_MMU_POWER_CTRL_REG(0), SPI_MEM_MMU_PAGE_SIZE);
    s_cache_state.ctrl = REG_READ(EXTMEM_L1_CACHE_CTRL_REG);
    s_cache_state.cache_was_enabled = is_cache_enabled();

    STUB_LOGD("cache_was_enabled: %d, mmu_page_size: %d\n",
              s_cache_state.cache_was_enabled,
              s_cache_state.mmu_page_size);

    if (!s_cache_state.cache_was_enabled) {
        STUB_LOGD("ICache not enabled, initializing for DROM\n");
        ROM_Boot_Cache_Init();
    }
}

void stub_target_cache_restore(void)
{
    if (!s_cache_state.cache_was_enabled) {
        STUB_LOGD("Disabling ICache\n");
        Cache_Disable_ICache();
        REG_WRITE(EXTMEM_L1_CACHE_CTRL_REG, s_cache_state.ctrl);
    } else {
        /* Restore MMU page size */
        REG_SET_FIELD(SPI_MEM_MMU_POWER_CTRL_REG(0), SPI_MEM_MMU_PAGE_SIZE, s_cache_state.mmu_page_size);
    }
}

int stub_target_cache_is_enabled(void)
{
    s_cache_state.ctrl = REG_READ(EXTMEM_L1_CACHE_CTRL_REG);
    return is_cache_enabled();
}

int stub_target_cache_init(void)
{
    ROM_Boot_Cache_Init();

    return 0;
}
