/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <esp-stub-lib/usb_serial_jtag.h>
#include <target/usb_serial_jtag.h>

extern void *GetUartDevice(void);
extern void esp_rom_uart_flush_tx(int uart_no);
extern uint8_t esp_rom_uart_tx_one_char(uint8_t ch);

typedef struct {
    int baud_rate;
    int data_bits;
    int exist_parity;
    int parity;
    int stop_bits;
    int flow_ctrl;
    uint8_t buff_uart_no;
    // ... following fields not needed
} UartDevice;

bool stub_lib_usb_serial_jtag_is_active(void)
{
    if (!stub_target_usb_serial_jtag_is_supported()) {
        return false;
    }

    UartDevice *uart = (UartDevice *)GetUartDevice();
    if (uart == NULL) {
        return false;
    }
    return (uart->buff_uart_no == stub_target_usb_serial_jtag_get_num());
}

void stub_lib_usb_serial_jtag_rominit_intr_attach(int intr_num, void *handler, uint32_t flags)
{
    stub_target_usb_serial_jtag_rominit_intr_attach(intr_num, handler, flags);
}

uint32_t stub_lib_usb_serial_jtag_get_intr_flags(void)
{
    return stub_target_usb_serial_jtag_get_intr_flags();
}

bool stub_lib_usb_serial_jtag_is_data_available(void)
{
    return stub_target_usb_serial_jtag_is_data_available();
}

uint8_t stub_lib_usb_serial_jtag_read_rxfifo_byte(void)
{
    return stub_target_usb_serial_jtag_read_rxfifo_byte();
}

uint8_t stub_lib_usb_serial_jtag_tx_one_char(uint8_t c)
{
    static unsigned short transferred_without_flush = 0;
    esp_rom_uart_tx_one_char(c);
    ++transferred_without_flush;
    if (c == '\xc0' || transferred_without_flush >= 63) {
        stub_lib_usb_serial_jtag_tx_flush();
        transferred_without_flush = 0;
    }
    return 0;
}

void stub_lib_usb_serial_jtag_tx_flush(void)
{
    esp_rom_uart_flush_tx(stub_target_usb_serial_jtag_get_num());
}
