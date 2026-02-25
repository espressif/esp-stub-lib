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

#define MMU_TABLE                         ((volatile uint32_t *)DR_REG_MMU_TABLE)

/*
 * ESP32-S3: IBUS flash MMU uses up to CACHE_DROM_MMU_MAX_END (0x400) bytes of
 * MMU entries (256 × uint32_t) for IROM+DROM together, 64 KiB pages. The first
 * two entries map IROM; DROM uses half-open indices [2, 256), matching the
 * usual IDF split (Cache_Set_IDROM_MMU_Size in cpu_start) and the legacy stub.
 *
 * Do not use Cache_Get_IROM_MMU_End() / Cache_Get_DROM_MMU_End(): ROM returns
 * byte offsets from RAM updated only by Cache_Set_IDROM_MMU_Size(), not from
 * hardware. Those values can be wrong (e.g. ROM defaults) before IDF startup
 * or out of sync with stub cache re-init, which breaks mmap entry math.
 */
#define ESP32S3_STUB_MMU_DROM_ENTRY_START 2U
#define ESP32S3_STUB_MMU_DROM_ENTRY_END   256U

uint32_t stub_target_mmu_get_drom_entry_start(void)
{
    return ESP32S3_STUB_MMU_DROM_ENTRY_START;
}

uint32_t stub_target_mmu_get_drom_entry_end(void)
{
    return ESP32S3_STUB_MMU_DROM_ENTRY_END;
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
