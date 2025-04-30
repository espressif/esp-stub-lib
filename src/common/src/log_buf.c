/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <stdint.h>
#include <stddef.h>

#include "log.h"

extern void ets_printf(const char *fmt, ...);
extern void ets_install_putc1(void (*p)(char c));
extern void ets_install_putc2(void (*p)(char c));

#if !defined(STUB_LIB_LOG_BUF_SIZE)
#define STUB_LIB_LOG_BUF_SIZE 4096
#endif

struct stub_lib_log_buf {
    uint32_t count;
    char buf[STUB_LIB_LOG_BUF_SIZE];
};

static struct stub_lib_log_buf g_stub_lib_log_buf;
static void stub_lib_log_buf_write_char(char c)
{
    g_stub_lib_log_buf.buf[g_stub_lib_log_buf.count] = c;
    g_stub_lib_log_buf.count = (g_stub_lib_log_buf.count + 1) % STUB_LIB_LOG_BUF_SIZE;
}

void stub_lib_log_init()
{
    ets_install_putc1(stub_lib_log_buf_write_char);
    ets_install_putc2(NULL);
}
