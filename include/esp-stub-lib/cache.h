/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief Write back the entire cache.
 *
 */
void stub_lib_cache_writeback_all(void);

/**
 * @brief Invalidate the entire cache.
 *
 */
void stub_lib_cache_invalidate_all(void);

/**
 * @brief Invalidate cache lines for a given address range.
 *
 * @param addr  Virtual address of the region to invalidate.
 * @param size  Size of the region in bytes.
 */
void stub_lib_cache_invalidate_addr(uint32_t vaddr, uint32_t size);

/**
 * @brief Write back cache lines for a given address range.
 *
 * @param addr  Virtual address of the region to write back.
 * @param size  Size of the region in bytes.
 */
void stub_lib_cache_writeback_addr(uint32_t vaddr, uint32_t size);

/**
 * @brief Suspend the cache and return the autoload state.
 *
 * @return Autoload state to pass to stub_lib_cache_resume().
 */
uint32_t stub_lib_cache_suspend(void);

/**
 * @brief Resume the cache with the given autoload state.
 *
 * Restores the autoload state captured by stub_lib_cache_suspend().
 *
 * @param autoload  Autoload state returned by stub_lib_cache_suspend().
 */
void stub_lib_cache_resume(uint32_t autoload);

/**
 * @brief Save the cache/MMU state and suspend the cache.
 *
 * Must be called before any flash operation that requires the cache to be off.
 * Pairs with stub_lib_cache_restore().
 *
 * @param[out] state_out  Receives an opaque pointer to the saved state.
 *                        Pass this unchanged to stub_lib_cache_restore().
 *                        Must not be NULL.
 */
void stub_lib_cache_save(const void **state_out);

/**
 * @brief Restore the cache/MMU state and resume the cache.
 *
 * Restores the exact state captured by stub_lib_cache_save().
 * Must be called after flash operations are complete.
 *
 * @param state  Pointer returned by stub_lib_cache_save().
 */
void stub_lib_cache_restore(const void *state);

#ifdef __cplusplus
}
#endif // __cplusplus
