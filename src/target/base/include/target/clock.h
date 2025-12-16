/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>

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
