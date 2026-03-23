/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <target/mmu.h>

uint32_t __attribute__((weak)) stub_target_mmu_get_drom_vaddr(void)
{
    return 0;
}

uint32_t __attribute__((weak)) stub_target_mmu_get_drom_entry_start(void)
{
    return 0;
}

uint32_t __attribute__((weak)) stub_target_mmu_get_drom_entry_end(void)
{
    return 0;
}

void __attribute__((weak)) stub_target_mmu_write_entry(uint32_t entry_id, uint32_t flash_page_num)
{
    (void)entry_id;
    (void)flash_page_num;
}

uint32_t __attribute__((weak)) stub_target_mmu_read_entry(uint32_t entry_id)
{
    (void)entry_id;
    return 0;
}

void __attribute__((weak)) stub_target_mmu_set_entry_invalid(uint32_t entry_id)
{
    (void)entry_id;
}

void __attribute__((weak)) stub_target_mmu_restore_entry(uint32_t entry_id, uint32_t raw_value)
{
    (void)entry_id;
    (void)raw_value;
}
