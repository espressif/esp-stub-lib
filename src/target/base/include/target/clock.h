/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define MHZ 1000000

/**
 * @brief Initialize clock
 */
void stub_target_clock_init(void);

/**
 * @brief Get CPU frequency
 *
 * @return CPU frequency in Hz
 */
uint32_t stub_target_get_cpu_freq(void);

/**
 * @brief Get APB frequency
 *
 * @return APB frequency in Hz
 */
uint32_t stub_target_get_apb_freq(void);

/**
 * @brief Disable watchdogs
 *
 * This function disables the watchdogs to prevent the device from resetting.
 * By default watchdog is disabled after chip reset, but second stage bootloader may enable it again,
 * and if application does not disable it, it will reset the device.
 */
void stub_target_clock_disable_watchdogs(void);

#ifdef __cplusplus
}
#endif // __cplusplus
