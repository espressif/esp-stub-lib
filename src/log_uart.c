/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <stdint.h>
#include <stddef.h>

#include <target/uart.h>

extern void ets_install_putc1(void (*p)(char c));
extern void ets_install_putc2(void (*p)(char c));
extern void ets_install_uart_printf(void);

void stub_lib_log_init()
{
    stub_target_uart_init(0, 115200);
    ets_install_putc1(NULL);
    ets_install_putc2(NULL);
    ets_install_uart_printf();
}
