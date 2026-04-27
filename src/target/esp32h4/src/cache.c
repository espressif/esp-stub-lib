/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stddef.h>
#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>
#include <target/mmu.h>

#include <soc/cache_reg.h>
#include <soc/spi_mem_compat.h>

extern uint32_t Cache_Suspend_Cache(uint32_t map);
extern void Cache_Resume_Cache(uint32_t map, uint32_t autoload);
extern void Cache_Invalidate_All(uint32_t map);
extern int Cache_Invalidate_Addr(uint32_t map, uint32_t addr, uint32_t size);
extern void Cache_WriteBack_All(void);
extern int Cache_WriteBack_Addr(uint32_t addr, uint32_t size);

#define CACHE_MAP_ICACHE0 BIT(0)
#define CACHE_MAP_ICACHE1 BIT(1)
#define CACHE_MAP_DCACHE  BIT(4)
#define CACHE_MAP_ALL     (CACHE_MAP_ICACHE0 | CACHE_MAP_ICACHE1 | CACHE_MAP_DCACHE)

typedef struct {
    uint32_t mmu_page_size;
} esp32h4_cache_state_t;

size_t stub_target_cache_state_size(void)
{
    return sizeof(esp32h4_cache_state_t);
}

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

/* State packing: raw autoload value from Cache_Suspend_Cache(CACHE_MAP_ALL) */
uint32_t stub_target_cache_suspend(void)
{
    return Cache_Suspend_Cache(CACHE_MAP_ALL);
}

void stub_target_cache_resume(uint32_t autoload)
{
    Cache_Resume_Cache(CACHE_MAP_ALL, autoload);
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
    if (state) {
        esp32h4_cache_state_t *s = state;
        s->mmu_page_size = REG_GET_FIELD(SPI_MEM_MMU_POWER_CTRL_REG(0), SPI_MMU_PAGE_SIZE);
    }
}

void stub_target_cache_deinit(const void *state)
{
    if (!state)
        return;

    const esp32h4_cache_state_t *s = state;

    /* Restore MMU page size */
    REG_SET_FIELD(SPI_MEM_MMU_POWER_CTRL_REG(0), SPI_MMU_PAGE_SIZE, s->mmu_page_size);
}
