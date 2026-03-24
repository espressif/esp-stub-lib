/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <target/mmu.h>

#include <soc/ext_mem_defs.h>
#include <soc/ext_mem_reg.h>
#include <soc/soc.h>

#define MMU_TABLE ((volatile uint32_t *)DR_REG_MMU_TABLE)

extern uint32_t Cache_Get_IROM_MMU_End(void);
extern uint32_t Cache_Get_DROM_MMU_End(void);

uint32_t stub_target_mmu_get_drom_vaddr(void)
{
    /* Unified MMU table: entry N maps to SOC_DROM_LOW + N * PAGE_SIZE,
     * so the DROM region starts at an offset from SOC_DROM_LOW. */
    return SOC_DROM_LOW + Cache_Get_IROM_MMU_End() / sizeof(uint32_t) * 0x10000U;
}

uint32_t stub_target_mmu_get_drom_entry_start(void)
{
    return Cache_Get_IROM_MMU_End() / sizeof(uint32_t);
}

uint32_t stub_target_mmu_get_drom_entry_end(void)
{
    return Cache_Get_DROM_MMU_End() / sizeof(uint32_t);
}

void stub_target_mmu_write_entry(uint32_t entry_id, uint32_t flash_page_num)
{
    MMU_TABLE[entry_id] = flash_page_num;
}

uint32_t stub_target_mmu_read_entry(uint32_t entry_id)
{
    return MMU_TABLE[entry_id];
}

void stub_target_mmu_set_entry_invalid(uint32_t entry_id)
{
    MMU_TABLE[entry_id] = SOC_MMU_INVALID;
}

void stub_target_mmu_restore_entry(uint32_t entry_id, uint32_t raw_value)
{
    MMU_TABLE[entry_id] = raw_value;
}
