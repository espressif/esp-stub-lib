/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <stddef.h>
#include <stdint.h>

#include <target/uart.h>

extern void ets_install_putc1(void (*p)(char c));
extern void ets_install_putc2(void (*p)(char c));
extern void ets_install_uart_printf(void);
extern void ets_install_usb_printf(void);

#if !defined(STUB_LIB_LOG_UART_NUM)
#define STUB_LIB_LOG_UART_NUM 0
#endif

void stub_lib_log_backend_init(void)
{
#if defined(STUB_LIB_LOG_TO_USB_SERIAL_JTAG) && (STUB_LIB_LOG_TO_USB_SERIAL_JTAG)
    ets_install_putc1(NULL);
    ets_install_putc2(NULL);
    ets_install_usb_printf();
#else
    stub_target_uart_init((uint8_t)STUB_LIB_LOG_UART_NUM);
    ets_install_putc1(NULL);
    ets_install_putc2(NULL);
    ets_install_uart_printf();
#endif
}
