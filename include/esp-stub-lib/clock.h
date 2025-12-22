/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
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

#ifdef __cplusplus
}
#endif // __cplusplus
