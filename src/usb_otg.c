/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <esp-stub-lib/usb_otg.h>
#include <target/usb_otg.h>
#include <esp_rom_caps.h>
#include <soc_utils.h>
#include <rom_wrappers.h>

#if (ESP_ROM_USB_OTG_NUM >= 0)

extern void *esp_rom_get_uart_device(void);
extern void esp_rom_isr_unmask(int int_num);
extern void esp_rom_isr_mask(int int_num);

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

// Constants
#define ACM_BYTES_PER_TX 64
#define LINE_CTRL_RTS (1 << 1)
#define ACM_STATUS_LINESTATE_CHANGED -1
#define ACM_STATUS_RX -4

// State
static int s_usb_int_num = 0;
static void (*s_rx_callback)(uint8_t) = NULL;
static char s_cdcacm_txbuf[ACM_BYTES_PER_TX];
static int s_cdcacm_txpos = 0;
static uint32_t s_cdcacm_old_rts = 0;
static volatile bool s_reset_requested = false;

// USB-OTG specific CDC ACM device type and functions
typedef void cdc_acm_device;
extern cdc_acm_device *uart_acm_dev;
extern int esp_rom_cdc_acm_fifo_fill(cdc_acm_device *dev, const uint8_t *tx_data, int len);
extern int esp_rom_cdc_acm_fifo_read(cdc_acm_device *dev, uint8_t *rx_data, const int size);
extern int esp_rom_cdc_acm_rx_fifo_cnt(cdc_acm_device *dev);
extern int esp_rom_cdc_acm_line_ctrl_get(cdc_acm_device *dev, uint32_t ctrl, uint32_t *val);
extern void esp_rom_cdc_acm_irq_rx_enable(cdc_acm_device *dev);
extern void esp_rom_cdc_acm_irq_rx_disable(cdc_acm_device *dev);

bool stub_lib_usb_otg_is_active(void)
{
    if (!stub_target_usb_otg_is_supported()) {
        return false;
    }

    uart_device_t *uart = (uart_device_t *)esp_rom_get_uart_device();
    if (uart == NULL) {
        return false;
    }
    return ((int8_t)uart->buff_uart_no == ESP_ROM_USB_OTG_NUM);
}

void stub_lib_usb_otg_cdcacm_callback(cdc_acm_device *dev, int status)
{
    if (status == ACM_STATUS_RX) {
        while (esp_rom_cdc_acm_rx_fifo_cnt(dev) > 0) {
            uint8_t c;
            esp_rom_cdc_acm_fifo_read(dev, &c, 1);
            s_rx_callback(c);
        }
    } else if (status == ACM_STATUS_LINESTATE_CHANGED) {
        uint32_t rts = 0;
        esp_rom_cdc_acm_line_ctrl_get(dev, LINE_CTRL_RTS, &rts);
        if (rts == 0 && s_cdcacm_old_rts == 1) {
            s_reset_requested = true;
        }
        s_cdcacm_old_rts = rts;
    }
}

void stub_lib_usb_otg_rominit_intr_attach(int intr_num, void (*rx_callback)(uint8_t))
{
    s_usb_int_num = intr_num;
    s_rx_callback = rx_callback;
    s_cdcacm_txpos = 0;
    s_reset_requested = false;

    // Get initial RTS state
    esp_rom_cdc_acm_line_ctrl_get(uart_acm_dev, LINE_CTRL_RTS, &s_cdcacm_old_rts);

    // Attach interrupt handler
    stub_target_usb_otg_rominit_intr_attach(intr_num, &stub_lib_usb_otg_cdcacm_callback);
}

uint8_t stub_lib_usb_otg_tx_one_char(uint8_t c)
{
    s_cdcacm_txbuf[s_cdcacm_txpos++] = c;
    if (s_cdcacm_txpos == sizeof(s_cdcacm_txbuf)) {
        stub_lib_usb_otg_tx_flush();
    }
    return 0;
}

void stub_lib_usb_otg_tx_flush(void)
{
    if (s_cdcacm_txpos > 0) {
        esp_rom_cdc_acm_fifo_fill(uart_acm_dev, (const uint8_t *)s_cdcacm_txbuf, s_cdcacm_txpos);
        s_cdcacm_txpos = 0;
    }
}

void stub_lib_usb_otg_rx_async_enable(bool enable)
{
    if (enable) {
        esp_rom_cdc_acm_irq_rx_enable(uart_acm_dev);
        esp_rom_isr_unmask(1 << s_usb_int_num);
    } else {
        esp_rom_isr_mask(1 << s_usb_int_num);
        esp_rom_cdc_acm_irq_rx_disable(uart_acm_dev);
    }
}

bool stub_lib_usb_otg_is_reset_requested(void)
{
    return s_reset_requested;
}

void stub_lib_usb_otg_handle_reset(void)
{
    s_reset_requested = false;
    esp_rom_isr_mask(1 << s_usb_int_num);

    // Wait a bit for USB to settle
    stub_lib_delay_us(10000);

    stub_target_usb_otg_handle_reset();
}

#else // (ESP_ROM_USB_OTG_NUM < 0)

// Chips without USB-OTG CDC in ROM: keep API but make everything a safe no-op.

bool stub_lib_usb_otg_is_active(void)
{
    return false;
}

void stub_lib_usb_otg_rominit_intr_attach(int intr_num, void (*rx_callback)(uint8_t))
{
    (void)intr_num;
    (void)rx_callback;
}

uint8_t stub_lib_usb_otg_tx_one_char(uint8_t c)
{
    (void)c;
    return 0;
}

void stub_lib_usb_otg_tx_flush(void)
{
}

void stub_lib_usb_otg_rx_async_enable(bool enable)
{
    (void)enable;
}

bool stub_lib_usb_otg_is_reset_requested(void)
{
    return false;
}

void stub_lib_usb_otg_handle_reset(void)
{
}

#endif
