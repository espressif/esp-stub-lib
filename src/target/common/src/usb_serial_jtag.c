/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>
#include <target/usb_serial_jtag.h>

bool __attribute__((weak)) stub_target_usb_serial_jtag_is_supported(void)
{
    return false;
}

void __attribute__((weak)) stub_target_usb_serial_jtag_rominit_intr_attach(int intr_num, void *handler, uint32_t flags)
{
    (void)intr_num;
    (void)handler;
    (void)flags;
}
