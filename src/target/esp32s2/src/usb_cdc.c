/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <target/usb_cdc.h>
#include <target/usb_cdc_common.h>
#include <esp_rom_caps.h>
#include <soc_utils.h>
#include <soc/reg_base.h>

// USB CDC ACM device type and functions (needed for direct calls in init)
typedef void cdc_acm_device;
extern cdc_acm_device *uart_acm_dev;
extern void cdc_acm_irq_callback_set(cdc_acm_device *dev, void (*cb)(cdc_acm_device *dev, int status));
extern void cdc_acm_irq_rx_enable(cdc_acm_device *dev);
extern void cdc_acm_irq_state_enable(cdc_acm_device *dev);

// Constants
#define USB_GAHBCFG_REG_OFFSET 0x008
#define USB_GLBLLNTRMSK (1 << 0)

// Chip-specific configuration
static const usb_cdc_chip_config_t usb_cdc_chip_config = {
    .usb_base_reg = 0x60080000,
    .rtc_base_reg = 0x3f408000
};

#define ETS_USB_INUM 9
#define ETS_USB_INTR_SOURCE 48

bool stub_target_usb_cdc_is_supported(void)
{
    return true;
}

uint8_t stub_target_usb_cdc_get_otg_num(void)
{
    return ESP_ROM_USB_OTG_NUM;
}

void stub_target_usb_cdc_init(void (*rx_callback)(uint8_t))
{
    // Initialize common state
    usb_cdc_common_init_state(rx_callback);

    // ESP32S2: Route interrupt using ROM function
    extern void esp_rom_route_intr_matrix(int cpu_no, int periph_src, int cpu_intr_num);
    esp_rom_route_intr_matrix(0, ETS_USB_INTR_SOURCE, ETS_USB_INUM);

    // Attach and enable ISR
    extern void usb_dw_isr_handler(void *arg);
    usb_cdc_common_attach_isr(ETS_USB_INUM, usb_dw_isr_handler);

    // Common setup
    cdc_acm_irq_callback_set(uart_acm_dev, &usb_cdc_common_callback);
    cdc_acm_irq_rx_enable(uart_acm_dev);
    cdc_acm_irq_state_enable(uart_acm_dev);

    // Enable USB global interrupt mask
    uint32_t gahbcfg_reg = usb_cdc_chip_config.usb_base_reg + USB_GAHBCFG_REG_OFFSET;
    WRITE_PERI_REG(gahbcfg_reg, READ_PERI_REG(gahbcfg_reg) | USB_GLBLLNTRMSK);
}

uint8_t stub_target_usb_cdc_tx_one_char(uint8_t c)
{
    return usb_cdc_common_tx_one_char(c);
}

void stub_target_usb_cdc_tx_flush(void)
{
    usb_cdc_common_tx_flush();
}

uint8_t stub_target_usb_cdc_rx_one_char(void)
{
    return usb_cdc_common_rx_one_char();
}

void stub_target_usb_cdc_rx_async_enable(bool enable)
{
    usb_cdc_common_rx_async_enable(enable);
}

bool stub_target_usb_cdc_is_reset_requested(void)
{
    return usb_cdc_common_is_reset_requested();
}

void stub_target_usb_cdc_handle_reset(void)
{
    usb_cdc_common_handle_reset(&usb_cdc_chip_config);
}
