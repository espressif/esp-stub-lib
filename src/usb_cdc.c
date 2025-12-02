/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <esp-stub-lib/usb_cdc.h>
#include <target/usb_cdc.h>
#include <soc_utils.h>

// External ROM function to get UART device info
extern void* GetUartDevice(void);

// UartDevice structure from ROM (matches ROM structure)
typedef struct {
    int     baud_rate;
    int     data_bits;
    int     exist_parity;
    int     parity;
    int     stop_bits;
    int     flow_ctrl;
    uint8_t buff_uart_no;
    // ... other fields not needed
} UartDevice;

// USB CDC state
static bool s_usb_cdc_active = false;
static volatile bool s_reset_requested = false;

bool stub_lib_usb_cdc_is_active(void)
{
    // Check if USB CDC is supported on this chip
    if (!stub_target_usb_cdc_is_supported()) {
        return false;
    }

    // Check if ROM has initialized USB CDC
    UartDevice *uart = (UartDevice *)GetUartDevice();
    if (uart == NULL) {
        return false;
    }

    uint8_t usb_otg_num = stub_target_usb_cdc_get_otg_num();
    return (uart->buff_uart_no == usb_otg_num);
}

void stub_lib_usb_cdc_init(void (*rx_callback)(uint8_t))
{
    s_usb_cdc_active = stub_lib_usb_cdc_is_active();

    if (s_usb_cdc_active) {
        stub_target_usb_cdc_init(rx_callback);
    }
}

uint8_t stub_lib_usb_cdc_tx_one_char(uint8_t c)
{
    if (!s_usb_cdc_active) {
        return 1;  // Error: USB CDC not active
    }
    return stub_target_usb_cdc_tx_one_char(c);
}

void stub_lib_usb_cdc_tx_flush(void)
{
    if (!s_usb_cdc_active) {
        return;
    }
    stub_target_usb_cdc_tx_flush();
}

uint8_t stub_lib_usb_cdc_rx_one_char(void)
{
    if (!s_usb_cdc_active) {
        return 0;
    }
    return stub_target_usb_cdc_rx_one_char();
}

void stub_lib_usb_cdc_rx_async_enable(bool enable)
{
    if (!s_usb_cdc_active) {
        return;
    }
    stub_target_usb_cdc_rx_async_enable(enable);
}

bool stub_lib_usb_cdc_is_reset_requested(void)
{
    if (!s_usb_cdc_active) {
        return false;
    }
    return s_reset_requested || stub_target_usb_cdc_is_reset_requested();
}

void stub_lib_usb_cdc_handle_reset(void)
{
    if (!s_usb_cdc_active) {
        return;
    }
    stub_target_usb_cdc_handle_reset();
    s_reset_requested = false;
}
