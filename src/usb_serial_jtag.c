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
#include <esp_rom_caps.h>
#include <soc_utils.h>

#if (ESP_ROM_USB_SERIAL_DEVICE_NUM >= 0)
#include <soc/usb_serial_jtag_reg.h>
#endif

extern void *esp_rom_get_uart_device(void);
extern void esp_rom_uart_flush_tx(uint8_t uart_no);
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
} uart_device_t;

bool stub_lib_usb_serial_jtag_is_active(void)
{
    if (!stub_target_usb_serial_jtag_is_supported()) {
        return false;
    }

    uart_device_t *uart = (uart_device_t *)esp_rom_get_uart_device();
    if (uart == NULL) {
        return false;
    }
    return ((int8_t)uart->buff_uart_no == ESP_ROM_USB_SERIAL_DEVICE_NUM);
}

void stub_lib_usb_serial_jtag_rominit_intr_attach(int intr_num, void *handler, uint32_t flags)
{
    stub_target_usb_serial_jtag_rominit_intr_attach(intr_num, handler, flags);
}

uint32_t stub_lib_usb_serial_jtag_clear_intr_flags(void)
{
#if (ESP_ROM_USB_SERIAL_DEVICE_NUM >= 0)
    uint32_t status = READ_PERI_REG(USB_SERIAL_JTAG_INT_ST_REG);
    // Clear the interrupts
    WRITE_PERI_REG(USB_SERIAL_JTAG_INT_CLR_REG, status);
    return status;
#else
    return 0;
#endif
}

bool stub_lib_usb_serial_jtag_is_data_available(void)
{
#if (ESP_ROM_USB_SERIAL_DEVICE_NUM >= 0)
    return READ_PERI_REG(USB_SERIAL_JTAG_EP1_CONF_REG) & USB_SERIAL_JTAG_SERIAL_OUT_EP_DATA_AVAIL;
#else
    return false;
#endif
}

uint8_t stub_lib_usb_serial_jtag_read_rxfifo_byte(void)
{
#if (ESP_ROM_USB_SERIAL_DEVICE_NUM >= 0)
    return READ_PERI_REG(USB_SERIAL_JTAG_EP1_REG) & 0xFF;
#else
    return 0;
#endif
}

uint8_t stub_lib_usb_serial_jtag_tx_one_char(uint8_t c)
{
    return esp_rom_uart_tx_one_char(c);
}

void stub_lib_usb_serial_jtag_tx_flush(void)
{
#if (ESP_ROM_USB_SERIAL_DEVICE_NUM >= 0)
    esp_rom_uart_flush_tx(ESP_ROM_USB_SERIAL_DEVICE_NUM);
#endif
}
