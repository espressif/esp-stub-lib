/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>

#include <soc/ext_mem_reg.h>
#include <soc/spi_mem_compat.h>

extern void Cache_Invalidate_All(void);
extern void Cache_WriteBack_All(void);
extern uint32_t Cache_Suspend_Cache(void);
extern void Cache_Resume_Cache(uint32_t autoload);

typedef struct {
    uint32_t mmu_page_size;
} esp32c61_cache_state_t;

static esp32c61_cache_state_t s_cache_state;

void stub_target_cache_writeback_all(void)
{
    Cache_WriteBack_All();
}

void stub_target_cache_invalidate_all(void)
{
    Cache_Invalidate_All();
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
    Cache_Resume_Cache(autoload & BIT(0) ? EXTMEM_L1_CACHE_AUTOLOAD_ENA : 0);
}

void stub_target_cache_save(const void **state_out)
{
    s_cache_state.mmu_page_size = REG_GET_FIELD(SPI_MEM_MMU_POWER_CTRL_REG(0), SPI_MMU_PAGE_SIZE);

    *state_out = &s_cache_state;
}

void stub_target_cache_restore(const void *state)
{
    const esp32c61_cache_state_t *s = state;

    /* Restore MMU page size */
    REG_SET_FIELD(SPI_MEM_MMU_POWER_CTRL_REG(0), SPI_MMU_PAGE_SIZE, s->mmu_page_size);
}
