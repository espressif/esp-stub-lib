/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>

#define STUB_CACHE_CAP_HAS_INVALIDATE_ADDR BIT(0)
#define STUB_CACHE_CAP_SHARED_IDCACHE      BIT(1)

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief Get cache capability flags for the current target.
 *
 * @return Bitmask of STUB_CACHE_CAP_* flags.
 */
uint32_t stub_lib_cache_get_caps(void);

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
 * @brief Save the cache/MMU state and initialize the cache for flash access.
 *
 * Must be called before any flash operation. Saves the current cache state into
 * @p state and, if the cache was not already enabled, initializes it for DROM access.
 * Pairs with stub_lib_cache_deinit().
 *
 * @param state  Out-pointer that receives the saved state. Pass back to stub_lib_cache_deinit().
 */
void stub_lib_cache_init(void **state);

/**
 * @brief Restore the cache/MMU state saved by stub_lib_cache_init().
 *
 * @param state  State pointer returned by stub_lib_cache_init(). If NULL, this is a no-op.
 */
void stub_lib_cache_deinit(const void *state);

/**
 * @brief Check if the cache is enabled.
 *
 * @return true if the cache is enabled, false otherwise.
 */
int stub_lib_is_cache_enabled(void);

#ifdef __cplusplus
}
#endif // __cplusplus
