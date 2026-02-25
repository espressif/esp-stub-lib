/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>

#include <target/usb_otg.h>

// Weak implementations for chips that don't support USB-OTG
bool __attribute__((weak)) stub_target_usb_otg_is_supported(void)
{
    return false;
}

void __attribute__((weak)) stub_target_usb_otg_rominit_intr_attach(int intr_num, void *callback)
{
    (void)callback;
    (void)intr_num;
}

void __attribute__((weak)) stub_target_usb_otg_handle_reset(void)
{
}
