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
#include <target/mmu.h>

#include <soc/cache_reg.h>
#include <soc/spi_mem_compat.h>

extern void ROM_Boot_Cache_Init(void);
extern void Cache_Disable_Cache(void);
extern void Cache_Invalidate_All(void);
extern void Cache_WriteBack_All(void);
extern int Cache_Invalidate_Addr(uint32_t addr, uint32_t size);
extern int Cache_WriteBack_Addr(uint32_t addr, uint32_t size);
extern uint32_t Cache_Suspend_Cache(void);
extern void Cache_Resume_Cache(uint32_t autoload);

typedef struct {
    uint32_t mmu_page_size;
    uint32_t ctrl;
    bool cache_was_enabled;
} esp32c5_cache_state_t;

static esp32c5_cache_state_t s_cache_state;

uint32_t stub_target_cache_get_caps(void)
{
    return STUB_CACHE_CAP_HAS_INVALIDATE_ADDR | STUB_CACHE_CAP_SHARED_IDCACHE;
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
    Cache_Resume_Cache(autoload & BIT(0) ? CACHE_L1_CACHE_AUTOLOAD_ENA : 0);
}

int stub_target_cache_is_enabled(void)
{
    uint32_t ctrl = REG_READ(CACHE_L1_CACHE_CTRL_REG);
    return stub_target_mmu_has_valid_entry() && !(ctrl & (CACHE_L1_CACHE_SHUT_BUS0 | CACHE_L1_CACHE_SHUT_BUS1));
}

void stub_target_cache_init(void **state)
{
    s_cache_state.mmu_page_size = REG_GET_FIELD(SPI_MEM_MMU_POWER_CTRL_REG(0), SPI_MMU_PAGE_SIZE);
    s_cache_state.ctrl = REG_READ(CACHE_L1_CACHE_CTRL_REG);
    s_cache_state.cache_was_enabled = stub_target_cache_is_enabled();

    STUB_LOGD("mmu_page_size: %d cache_ctrl: 0x%x\n", s_cache_state.mmu_page_size, s_cache_state.ctrl);

    if (!s_cache_state.cache_was_enabled) {
        STUB_LOGD("Cache not enabled, initializing for DROM\n");
        ROM_Boot_Cache_Init();
        REG_CLR_BIT(CACHE_L1_CACHE_CTRL_REG, CACHE_L1_CACHE_SHUT_BUS0);
    }

    if (state)
        *state = &s_cache_state;
}

void stub_target_cache_deinit(const void *state)
{
    if (!state)
        return;

    const esp32c5_cache_state_t *s = state;

    if (!s->cache_was_enabled) {
        STUB_LOGD("Disabling cache\n");
        Cache_Disable_Cache();
        REG_WRITE(CACHE_L1_CACHE_CTRL_REG, s->ctrl);
    } else {
        /* Restore MMU page size */
        REG_SET_FIELD(SPI_MEM_MMU_POWER_CTRL_REG(0), SPI_MMU_PAGE_SIZE, s->mmu_page_size);
    }
}
