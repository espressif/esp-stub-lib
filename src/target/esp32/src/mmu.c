/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <target/mmu.h>

#include <soc/dport_reg.h>
#include <soc/ext_mem_defs.h>
#include <soc/soc.h>

uint32_t stub_target_mmu_get_drom_vaddr(void)
{
    return SOC_DROM_LOW;
}

uint32_t stub_target_mmu_get_drom_entry_start(void)
{
    return SOC_MMU_DROM0_PAGES_START;
}

uint32_t stub_target_mmu_get_drom_entry_end(void)
{
    return SOC_MMU_DROM0_PAGES_END;
}

void stub_target_mmu_write_entry(uint32_t entry_id, uint32_t flash_page_num)
{
    DPORT_PRO_FLASH_MMU_TABLE[entry_id] = flash_page_num;
}

uint32_t stub_target_mmu_read_entry(uint32_t entry_id)
{
    return DPORT_PRO_FLASH_MMU_TABLE[entry_id];
}

void stub_target_mmu_set_entry_invalid(uint32_t entry_id)
{
    DPORT_PRO_FLASH_MMU_TABLE[entry_id] = SOC_MMU_INVALID;
}

void stub_target_mmu_restore_entry(uint32_t entry_id, uint32_t raw_value)
{
    DPORT_PRO_FLASH_MMU_TABLE[entry_id] = raw_value;
}
