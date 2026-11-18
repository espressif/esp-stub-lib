/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief Initialize clock to higher CPU frequency if possible
 */
void stub_lib_clock_init(void);

/**
 * @brief Disable watchdogs
 *
 * This function disables the watchdogs to prevent the device from resetting.
 * By default watchdog is disabled after chip reset, but second stage bootloader may enable it again,
 * and if application does not disable it, it will reset the device.
 */
void stub_lib_clock_disable_watchdogs(void);

/**
 * @brief Enable the CPU performance counter used by ROM delay functions
 *
 * The ROM start code enables this counter, but a debugger can halt the CPU at the
 * reset vector before that code runs. ROM delays then wait forever on a stopped counter.
 * Call it once at stub entry, before anything that may use a ROM delay.
 */
void stub_lib_clock_enable_perf_counter(void);

#ifdef __cplusplus
}
#endif // __cplusplus
