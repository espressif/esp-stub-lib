/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>

enum stub_lib_log_destination {
	STUB_LIB_LOG_DEST_NONE,
	STUB_LIB_LOG_DEST_UART, /* using default UART configuration */
	STUB_LIB_LOG_DEST_BUF, /* using a SRAM buffer for logging, that passed back via g_stub_lib_log_buf */
};

/* should be power of 2 */
#define STUB_LIB_LOG_BUF_SIZE 4096
struct stub_lib_log_buf {
	uint32_t count;
	char buf[STUB_LIB_LOG_BUF_SIZE];
};

void stub_lib_log_init(enum stub_lib_log_destination dest);
void stub_lib_log_printf(const char *fmt, ...);
