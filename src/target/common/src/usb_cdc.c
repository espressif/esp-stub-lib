/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>
#include <target/usb_cdc.h>

// Weak implementations for chips that don't support USB CDC
bool __attribute__((weak)) stub_target_usb_cdc_is_supported(void)
{
    return false;
}

uint8_t __attribute__((weak)) stub_target_usb_cdc_get_otg_num(void)
{
    return 0;
}

void __attribute__((weak)) stub_target_usb_cdc_init(void (*rx_callback)(uint8_t))
{
    (void)rx_callback;
}

uint8_t __attribute__((weak)) stub_target_usb_cdc_tx_one_char(uint8_t c)
{
    (void)c;
    return 0;  // Success
}

void __attribute__((weak)) stub_target_usb_cdc_tx_flush(void)
{
}

uint8_t __attribute__((weak)) stub_target_usb_cdc_rx_one_char(void)
{
    return 0;
}

void __attribute__((weak)) stub_target_usb_cdc_rx_async_enable(bool enable)
{
    (void)enable;
}

bool __attribute__((weak)) stub_target_usb_cdc_is_reset_requested(void)
{
    return false;
}

void __attribute__((weak)) stub_target_usb_cdc_handle_reset(void)
{
}
