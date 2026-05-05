/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <stddef.h>

#include <target/uart.h>

extern void ets_install_putc1(void (*p)(char c));
extern void ets_install_putc2(void (*p)(char c));
extern void ets_install_uart_printf(void);

#if !defined(STUB_LIB_LOG_UART_NUM)
#define STUB_LIB_LOG_UART_NUM 0
#endif

void stub_lib_log_backend_init(void)
{
    stub_target_uart_init(STUB_LIB_LOG_UART_NUM);
    ets_install_putc1(NULL);
    ets_install_putc2(NULL);
    ets_install_uart_printf();
}
