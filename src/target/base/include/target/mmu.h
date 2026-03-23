/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>

#include <esp-stub-lib/mmu.h>

/**
 * @brief Get the DROM virtual address base for mmap operations.
 *
 * @return Start of the DROM virtual address region (e.g. SOC_DROM_LOW).
 */
uint32_t stub_target_mmu_get_drom_vaddr(void);

/**
 * @brief Get the first MMU entry index in the DROM region.
 *
 * On chips with partitioned MMU tables (e.g. ESP32 where entries 0–63 are
 * DROM, 64–127 are IRAM0, etc.), this returns the first DROM entry index.
 * On chips with unified IROM/DROM tables, this typically returns 0.
 *
 * @return First DROM entry index (e.g. SOC_MMU_DROM0_PAGES_START).
 */
uint32_t stub_target_mmu_get_drom_entry_start(void);

/**
 * @brief Get the one-past-last MMU entry index in the DROM region.
 *
 * @return One past the last DROM entry index (e.g. SOC_MMU_DROM0_PAGES_END).
 */
uint32_t stub_target_mmu_get_drom_entry_end(void);

/**
 * @brief Write a single MMU entry mapping a flash page.
 *
 * The implementation must add the chip-specific valid bit(s).
 *
 * @param entry_id        MMU entry index.
 * @param flash_page_num  Flash page number (paddr >> STUB_MMU_PAGE_SHIFT).
 */
void stub_target_mmu_write_entry(uint32_t entry_id, uint32_t flash_page_num);

/**
 * @brief Read a single MMU entry (raw value for save/restore).
 *
 * @param entry_id  MMU entry index.
 * @return Raw MMU entry value.
 */
uint32_t stub_target_mmu_read_entry(uint32_t entry_id);

/**
 * @brief Set a single MMU entry to invalid.
 *
 * @param entry_id  MMU entry index.
 */
void stub_target_mmu_set_entry_invalid(uint32_t entry_id);

/**
 * @brief Restore a single MMU entry from a previously saved raw value.
 *
 * Unlike write_entry, this writes the raw value as-is (no valid bit added).
 *
 * @param entry_id   MMU entry index.
 * @param raw_value  Raw value previously returned by stub_target_mmu_read_entry().
 */
void stub_target_mmu_restore_entry(uint32_t entry_id, uint32_t raw_value);
