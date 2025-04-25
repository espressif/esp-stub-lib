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

void stub_lib_log_init(uint8_t uart_num, uint32_t baudrate);
void stub_lib_log_printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif // __cplusplus
