/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Check if address is in external memory range.
 *
 * External flash memory mapped via the instruction bus.
 */
bool stub_lib_mem_is_irom(uintptr_t addr);

/**
 * @brief Check if address is in external memory range.
 *
 * External flash memory mapped via the data bus.
 */
bool stub_lib_mem_is_drom(uintptr_t addr);

/**
 * @brief Check if address is in internal memory range.
 *
 * Internal memory accessed via the instruction bus.
 */
bool stub_lib_mem_is_iram(uintptr_t addr);

/**
 * @brief Check if address is in internal memory range.
 *
 * Internal memory accessed via the data bus.
 */
bool stub_lib_mem_is_dram(uintptr_t addr);

/**
 * @brief Check if the address is in internal RTC memory range.
 *
 * Chip-specific internal memory accessed via the instruction bus.
 */
bool stub_lib_mem_is_rtc_iram_fast(uintptr_t addr);

/**
 * @brief Check if the address is in internal RTC memory range.
 *
 * Chip-specific internal memory accessed via the data bus.
 */
bool stub_lib_mem_is_rtc_dram_fast(uintptr_t addr);

/**
 * @brief Check if address is in internal RTC memory range.
 *
 * Chip-specific internal memory accessed via both the instruction and data buses.
 */
bool stub_lib_mem_is_rtc_slow(uintptr_t addr);

/**
 * @brief Check if address is in internal TCM memory range.
 *
 * Chip-specific internal memory accessed via both the instruction and data buses.
 */
bool stub_lib_mem_is_tcm(uintptr_t addr);
