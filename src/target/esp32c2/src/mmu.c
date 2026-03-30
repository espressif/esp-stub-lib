/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <esp-stub-lib/soc_utils.h>

#include <target/mmu.h>

#include <soc/ext_mem_defs.h>
#include <soc/ext_mem_reg.h>
#include <soc/reg_base.h>
#include <soc/soc.h>

#define MMU_TABLE                         ((volatile uint32_t *)DR_REG_MMU_TABLE)

/*
 * ESP32-C2: 64 MMU entries (CACHE_DROM_MMU_MAX_END 0x100 bytes). First two map
 * IROM; DROM uses [2, 64). Matches typical IDF split; IROM entry count can
 * differ with smaller MMU page sizes (see stub_target_mmu_get_page_size()).
 *
 * Avoid Cache_Get_IROM_MMU_End() / Cache_Get_DROM_MMU_End(): ROM uses RAM
 * updated by Cache_Set_IDROM_MMU_Size(), not hardware — unreliable in the stub.
 */
#define ESP32C2_STUB_MMU_DROM_ENTRY_START 2U
#define ESP32C2_STUB_MMU_DROM_ENTRY_END   64U

uint32_t stub_target_mmu_get_page_size(void)
{
    uint32_t code = REG_GET_FIELD(EXTMEM_CACHE_CONF_MISC_REG, EXTMEM_CACHE_MMU_PAGE_SIZE);

    if (code == 0) {
        return 0x4000U;
    } else if (code == 1) {
        return 0x8000U;
    }
    return 0x10000U;
}

uint32_t stub_target_mmu_get_drom_entry_start(void)
{
    return ESP32C2_STUB_MMU_DROM_ENTRY_START;
}

uint32_t stub_target_mmu_get_drom_entry_end(void)
{
    return ESP32C2_STUB_MMU_DROM_ENTRY_END;
}

uint32_t stub_target_mmu_get_drom_vaddr(void)
{
    return SOC_DROM_LOW + stub_target_mmu_get_drom_entry_start() * stub_target_mmu_get_page_size();
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
