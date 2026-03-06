/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

/**
 * @brief Save the cache/MMU state and suspend the cache.
 *
 * Snapshots all MMU entries, page size, and bus-mask register, then suspends
 * the instruction cache.  The saved state is stored internally by the target
 * implementation (static storage).  Pass the returned pointer unchanged to
 * stub_target_cache_restore().
 *
 * @param[out] state_out  Set to point to the internal state on return.
 *                        Must not be NULL.
 */
void stub_target_cache_save(const void **state_out);

/**
 * @brief Restore the cache/MMU state and resume the cache.
 *
 * Writes back all MMU entries and control registers saved by
 * stub_target_cache_save(), invalidates the cache, then resumes it.
 *
 * @param state  Pointer previously written by stub_target_cache_save().
 */
void stub_target_cache_restore(const void *state);
