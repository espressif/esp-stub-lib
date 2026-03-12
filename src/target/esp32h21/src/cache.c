/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>

#include <soc/cache_reg.h>
#include <soc/spi_mem_compat.h>

extern uint32_t Cache_Suspend_ICache(void);
extern void Cache_Resume_ICache(uint32_t autoload);
extern void Cache_Invalidate_ICache_All(void);
extern int Cache_Invalidate_Addr(uint32_t addr, uint32_t size);

typedef struct {
    uint32_t mmu_page_size;
} esp32h21_cache_state_t;

static esp32h21_cache_state_t s_cache_state;

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

void stub_target_cache_save(const void **state_out)
{
    s_cache_state.mmu_page_size = REG_GET_FIELD(SPI_MEM_MMU_POWER_CTRL_REG(0), SPI_MEM_MMU_PAGE_SIZE);

    *state_out = &s_cache_state;
}

void stub_target_cache_restore(const void *state)
{
    const esp32h21_cache_state_t *s = state;

    /* Restore MMU page size */
    REG_SET_FIELD(SPI_MEM_MMU_POWER_CTRL_REG(0), SPI_MEM_MMU_PAGE_SIZE, s->mmu_page_size);
}
