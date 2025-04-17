/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#if defined(STUB_LOG_ENABLED)

#if !defined(STUB_LIB_LOG_BUF) && !defined(STUB_LIB_LOG_UART)
#define STUB_LIB_LOG_BUF
#endif

#if defined(STUB_LIB_LOG_BUF)
#include <stdint.h>

/* should be power of 2 */
#define STUB_LIB_LOG_BUF_SIZE 4096
struct stub_lib_log_buf {
    uint32_t count;
    char buf[STUB_LIB_LOG_BUF_SIZE];
};
#endif // defined(STUB_LIB_LOG_BUF)

void stub_lib_log_init();
void stub_lib_log_printf(const char *fmt, ...);

#define STUB_LOG_INIT() stub_lib_log_init()
#define STUB_LOG(fmt, ...) stub_lib_log_printf(fmt, ##__VA_ARGS__)

#else // defined(STUB_LOG_ENABLED)

#define STUB_LOG_INIT()
#define STUB_LOG(fmt, ...)

#endif // defined(STUB_LOG_ENABLED)
