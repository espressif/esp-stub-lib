/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <target/usb_cdc_common.h>
#include <soc_utils.h>

// ROM function declarations
extern void esp_rom_isr_attach(int int_num, void *handler, void *arg);
extern void esp_rom_isr_unmask(int int_num);
extern void esp_rom_isr_mask(int int_num);
extern void usb_dc_check_poll_for_interrupts(void);
extern void chip_usb_set_persist_flags(uint32_t flags);
extern int usb_dc_prepare_persist(void);
extern void esp_rom_delay_us(uint32_t us);
extern void esp_rom_software_reset_cpu(uint32_t);
extern void usb_dw_isr_handler(void *arg);

// USB CDC ACM device type and functions
typedef void cdc_acm_device;
extern cdc_acm_device *uart_acm_dev;

typedef void (*uart_irq_callback_t)(cdc_acm_device *dev, int status);
extern void cdc_acm_irq_callback_set(cdc_acm_device *dev, uart_irq_callback_t cb);
extern void cdc_acm_irq_rx_enable(cdc_acm_device *dev);
extern void cdc_acm_irq_rx_disable(cdc_acm_device *dev);
extern int cdc_acm_fifo_read(cdc_acm_device *dev, uint8_t *rx_data, const int size);
extern int cdc_acm_fifo_fill(cdc_acm_device *dev, const uint8_t *tx_data, int len);
extern int cdc_acm_line_ctrl_get(cdc_acm_device *dev, uint32_t ctrl, uint32_t *val);
extern int cdc_acm_rx_fifo_cnt(cdc_acm_device *dev);
extern void cdc_acm_irq_state_enable(cdc_acm_device *dev);

// Constants
#define ACM_BYTES_PER_TX 64
#define ACM_STATUS_RX -4
#define ACM_STATUS_LINESTATE_CHANGED -1
#define LINE_CTRL_RTS (1 << 1)
#define USBDC_PERSIST_ENA (1U << 31)
#define RTC_CNTL_OPTION1_REG_OFFSET 0x0124
#define RTC_CNTL_FORCE_DOWNLOAD_BOOT (1 << 0)
#define USB_GAHBCFG_REG_OFFSET 0x008
#define USB_GLBLLNTRMSK (1 << 0)

// State
static void (*s_rx_callback)(uint8_t) = NULL;
static uint32_t s_cdcacm_old_rts = 0;
static volatile bool s_reset_requested = false;
static char s_cdcacm_txbuf[ACM_BYTES_PER_TX];
static size_t s_cdcacm_txpos = 0;
static uint8_t s_usb_int_num = 0;  // USB interrupt number (set during configure_interrupt)

void usb_cdc_common_init_state(void (*rx_callback)(uint8_t))
{
    s_rx_callback = rx_callback;
    s_cdcacm_txpos = 0;
    s_reset_requested = false;

    // Get initial RTS state
    uint32_t rts = 0;
    cdc_acm_line_ctrl_get(uart_acm_dev, LINE_CTRL_RTS, &rts);
    s_cdcacm_old_rts = rts;
}

void usb_cdc_common_callback(cdc_acm_device *dev, int status)
{
    if (status == ACM_STATUS_RX) {
        while (cdc_acm_rx_fifo_cnt(uart_acm_dev) > 0) {
            uint8_t c;
            cdc_acm_fifo_read(uart_acm_dev, &c, 1);
            if (s_rx_callback) {
                s_rx_callback(c);
            }
        }
    } else if (status == ACM_STATUS_LINESTATE_CHANGED) {
        uint32_t rts = 0;
        cdc_acm_line_ctrl_get(dev, LINE_CTRL_RTS, &rts);
        if (rts == 0 && s_cdcacm_old_rts == 1) {
            s_reset_requested = true;
        }
        s_cdcacm_old_rts = rts;
    }
}

void usb_cdc_common_flush(void)
{
    if (s_cdcacm_txpos > 0) {
        cdc_acm_fifo_fill(uart_acm_dev, (const uint8_t *)s_cdcacm_txbuf, (int)s_cdcacm_txpos);
        s_cdcacm_txpos = 0;
    }
}

uint8_t usb_cdc_common_tx_one_char(uint8_t c)
{
    s_cdcacm_txbuf[s_cdcacm_txpos++] = c;
    if (c == 0xC0 || s_cdcacm_txpos == sizeof(s_cdcacm_txbuf)) {
        usb_cdc_common_flush();
    }
    return 0;  // Success
}

void usb_cdc_common_tx_flush(void)
{
    usb_cdc_common_flush();
}

uint8_t usb_cdc_common_rx_one_char(void)
{
    // Blocking read - not typically used, but provided for compatibility
    uint8_t c = 0;
    while (cdc_acm_rx_fifo_cnt(uart_acm_dev) == 0) {
        // Wait for data
    }
    cdc_acm_fifo_read(uart_acm_dev, &c, 1);
    return c;
}

void usb_cdc_common_rx_async_enable(bool enable)
{
    if (enable) {
        cdc_acm_irq_rx_enable(uart_acm_dev);
        esp_rom_isr_unmask(1 << s_usb_int_num);
    } else {
        esp_rom_isr_mask(1 << s_usb_int_num);
        cdc_acm_irq_rx_disable(uart_acm_dev);
    }
}

bool usb_cdc_common_is_reset_requested(void)
{
    return s_reset_requested;
}

void usb_cdc_common_attach_isr(uint8_t int_num, void (*isr_handler)(void *arg))
{
    s_usb_int_num = int_num;
    esp_rom_isr_attach(int_num, isr_handler, NULL);
    esp_rom_isr_unmask(1 << int_num);
}

void usb_cdc_common_handle_reset(const usb_cdc_chip_config_t *config)
{
    s_reset_requested = false;
    esp_rom_isr_mask(1 << s_usb_int_num);

    // Wait a bit for USB to settle
    esp_rom_delay_us(10000);

    // Handle any pending interrupts
    usb_dc_check_poll_for_interrupts();

    // Clear force download boot flag
    uint32_t rtc_option1_reg = config->rtc_base_reg + RTC_CNTL_OPTION1_REG_OFFSET;
    WRITE_PERI_REG(rtc_option1_reg,
                   READ_PERI_REG(rtc_option1_reg) & ~(uint32_t)RTC_CNTL_FORCE_DOWNLOAD_BOOT);

    // Set USB persist flags
    chip_usb_set_persist_flags(USBDC_PERSIST_ENA);
    usb_dc_prepare_persist();

    // Reset
    esp_rom_software_reset_cpu(0);
}
