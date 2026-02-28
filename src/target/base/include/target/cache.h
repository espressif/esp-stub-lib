/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

/**
 * @brief Writeback dirty cache lines and invalidate the entire instruction cache.
 *
 * On chips without writeback support, only invalidates.
 */
void stub_target_cache_writeback_invalidate_all(void);

/**
 * @brief Invalidate the entire instruction cache.
 */
void stub_target_cache_invalidate_all(void);
