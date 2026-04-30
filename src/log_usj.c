/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <stddef.h>

extern void ets_install_putc1(void (*p)(char c));
extern void ets_install_putc2(void (*p)(char c));
extern void ets_install_usb_printf(void);

void stub_lib_log_backend_init(void)
{
    ets_install_putc1(NULL);
    ets_install_putc2(NULL);
    ets_install_usb_printf();
}
