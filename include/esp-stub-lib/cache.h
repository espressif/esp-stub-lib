/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stddef.h>
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
 * @brief Stop the cache (suspend or freeze, depending on target).
 *
 * Pairs with stub_lib_cache_start(). Targets that need to preserve
 * autoload/enable state across the stop/start pair keep it internally.
 */
void stub_lib_cache_stop(void);

/**
 * @brief Start the cache after a previous stub_lib_cache_stop().
 */
void stub_lib_cache_start(void);

/**
 * @brief Return the size in bytes of the target-specific cache state buffer.
 *
 * The caller must allocate at least this many bytes (aligned to uint32_t) and
 * pass the buffer to stub_lib_cache_init().
 * Returns 0 if the target does not require any state save/restore.
 */
size_t stub_lib_cache_state_size(void) __attribute__((const));

/**
 * @brief Save the cache/MMU state and initialize the cache for flash access.
 *
 * Saves the current cache state into @p state and,
 * if the cache was not already enabled, initializes it for DROM bus access.
 * Pairs with stub_lib_cache_deinit().
 *
 * @param state Pre-allocated buffer of at least stub_lib_cache_state_size() bytes,
 *              or NULL if no state save/restore is needed.
 */
void stub_lib_cache_init(void *state);

/**
 * @brief Restore the cache/MMU state saved by stub_lib_cache_init().
 *
 * @param state  Buffer previously passed to stub_lib_cache_init(). If NULL, this is a no-op.
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
