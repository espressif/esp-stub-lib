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
 * @brief Writeback dirty cache lines and invalidate the entire instruction cache.
 *
 * On chips without writeback support, only invalidates.
 * Call BEFORE reading flash via SPI to ensure cache coherency.
 */
void stub_lib_cache_writeback_invalidate_all(void);

/**
 * @brief Invalidate the entire instruction cache.
 *
 * Call AFTER modifying flash to ensure the CPU fetches updated instructions.
 */
void stub_lib_cache_invalidate_all(void);

#ifdef __cplusplus
}
#endif // __cplusplus
