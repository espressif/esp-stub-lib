/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>

#include <esp-stub-lib/cache.h>

/**
 * @brief Get cache capability flags for the current target.
 *
 * @return Bitmask of STUB_CACHE_CAP_* flags.
 */
uint32_t stub_target_cache_get_caps(void);

/**
 * @brief Write back the entire cache.
 *
 */
void stub_target_cache_writeback_all(void);

/**
 * @brief Invalidate the entire cache.
 *
 */
void stub_target_cache_invalidate_all(void);

/**
 * @brief Invalidate cache lines for a given address range.
 *
 * @param vaddr  Virtual address of the region to invalidate.
 * @param size   Size of the region in bytes.
 */
void stub_target_cache_invalidate_addr(uint32_t vaddr, uint32_t size);

/**
 * @brief Write back cache lines for a given address range.
 *
 * @param vaddr  Virtual address of the region to write back.
 * @param size   Size of the region in bytes.
 */
void stub_target_cache_writeback_addr(uint32_t vaddr, uint32_t size);

/**
 * @brief Suspend the cache and return the autoload state.
 *
 * @return Autoload state to pass to stub_target_cache_resume().
 */
uint32_t stub_target_cache_suspend(void);

/**
 * @brief Resume the cache with the given autoload state.
 *
 * @param autoload  Autoload state returned by stub_target_cache_suspend().
 */
void stub_target_cache_resume(uint32_t autoload);

/**
 * @brief Save the cache/MMU state
 *
 */
void stub_target_cache_save(void);

/**
 * @brief Restore the cache/MMU state
 *
 */
void stub_target_cache_restore(void);
