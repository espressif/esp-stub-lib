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

#ifdef __cplusplus
}
#endif // __cplusplus
