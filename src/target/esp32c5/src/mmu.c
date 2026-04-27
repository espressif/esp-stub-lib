/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <esp-stub-lib/log.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/mmu.h>
#include <target/security.h>

#include <soc/ext_mem_defs.h>
#include <soc/soc.h>
#include <soc/spi_mem_compat.h>

#define ESP32C5_STUB_MMU_DROM_ENTRY_START 2U

uint32_t stub_target_mmu_get_drom_entry_start(void)
{
    return ESP32C5_STUB_MMU_DROM_ENTRY_START;
}

uint32_t stub_target_mmu_get_drom_entry_end(void)
{
    return SOC_MMU_ENTRY_NUM;
}

uint32_t stub_target_mmu_get_drom_vaddr(void)
{
    return SOC_DROM_LOW + stub_target_mmu_get_drom_entry_start() * stub_target_mmu_get_page_size();
}

void stub_target_mmu_write_entry(uint32_t entry_id, uint32_t flash_page_num)
{
    uint32_t mmu_raw_value;

    if (stub_target_security_flash_is_encrypted())
        flash_page_num |= SOC_MMU_SENSITIVE;

    mmu_raw_value = flash_page_num | SOC_MMU_VALID;
    REG_WRITE(SPI_MEM_MMU_ITEM_INDEX_REG(0), entry_id);
    REG_WRITE(SPI_MEM_MMU_ITEM_CONTENT_REG(0), mmu_raw_value);
}

uint32_t stub_target_mmu_read_entry(uint32_t entry_id)
{
    REG_WRITE(SPI_MEM_MMU_ITEM_INDEX_REG(0), entry_id);
    uint32_t mmu_raw_value = REG_READ(SPI_MEM_MMU_ITEM_CONTENT_REG(0));
    if (stub_target_security_flash_is_encrypted())
        mmu_raw_value &= ~SOC_MMU_SENSITIVE;

    return mmu_raw_value;
}

void stub_target_mmu_set_entry_invalid(uint32_t entry_id)
{
    REG_WRITE(SPI_MEM_MMU_ITEM_INDEX_REG(0), entry_id);
    REG_WRITE(SPI_MEM_MMU_ITEM_CONTENT_REG(0), SOC_MMU_INVALID);
}

void stub_target_mmu_restore_entry(uint32_t entry_id, uint32_t raw_value)
{
    if (raw_value & SOC_MMU_VALID)
        stub_target_mmu_write_entry(entry_id, raw_value & SOC_MMU_VALID_VAL_MASK);
    else
        stub_target_mmu_set_entry_invalid(entry_id);
}

bool stub_target_mmu_has_valid_entry(void)
{
    for (uint32_t i = 0; i < SOC_MMU_ENTRY_NUM; ++i) {
        uint32_t mmu_raw_value = stub_target_mmu_read_entry(i);
        if ((mmu_raw_value & SOC_MMU_VALID) == SOC_MMU_VALID) {
            return true;
        }
    }
    return false;
}
