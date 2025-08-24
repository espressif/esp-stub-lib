/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdbool.h>

/**
 * @brief Check if address is in external memory range.
 *
 * External flash memory mapped via the instruction bus.
 */
bool stub_lib_mem_is_irom(const void *p);

/**
 * @brief Check if address is in external memory range.
 *
 * External flash memory mapped via the data bus.
 */
bool stub_lib_mem_is_drom(const void *p);

/**
 * @brief Check if address is in internal memory range.
 *
 * Internal memory accessed via the instruction bus.
 */
bool stub_lib_mem_is_iram(const void *p);

/**
 * @brief Check if address is in internal memory range.
 *
 * Internal memory accessed via the data bus.
 */
bool stub_lib_mem_is_dram(const void *p);

bool stub_lib_mem_is_rtc_iram_fast(const void *p);
bool stub_lib_mem_is_rtc_dram_fast(const void *p);
bool stub_lib_mem_is_rtc_slow(const void *p);

bool stub_lib_mem_is_tcm(const void *p);
