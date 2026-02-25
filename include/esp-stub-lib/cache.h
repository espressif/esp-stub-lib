/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

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
