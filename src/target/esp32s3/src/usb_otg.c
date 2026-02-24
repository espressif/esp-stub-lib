/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <esp_rom_caps.h>
#include <soc_utils.h>

#include <target/usb_otg.h>

#include <soc/interrupt_core0_reg.h>
#include <soc/reg_base.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/usb_reg.h>

typedef void cdc_acm_device;
extern cdc_acm_device *uart_acm_dev;

extern void esp_rom_cdc_acm_irq_callback_set(cdc_acm_device *dev, void (*cb)(cdc_acm_device *dev, int status));
extern void esp_rom_cdc_acm_irq_rx_enable(cdc_acm_device *dev);
extern void esp_rom_cdc_acm_irq_state_enable(cdc_acm_device *dev);
extern void esp_rom_route_intr_matrix(int cpu_no, int periph_src, int cpu_intr_num);
extern void esp_rom_isr_attach(int int_num, void *handler, void *arg);
extern void esp_rom_isr_unmask(int int_num);
extern void esp_rom_software_reset_cpu(uint32_t);
extern void esp_rom_usb_dc_check_poll_for_interrupts(void);
extern int esp_rom_usb_dc_prepare_persist(void);
extern void esp_rom_chip_usb_set_persist_flags(uint32_t flags);
extern void esp_rom_usb_dw_isr_handler(void *arg);

#define USBDC_PERSIST_ENA (1U << 31)

bool stub_target_usb_otg_is_supported(void)
{
    return true;
}

/* Workaround for LoadStoreException during compressed flashing over USB OTG.
 *
 * Without this wrapper, a LoadStoreException occurs during the first call to tinfl_decompress()
 * due to register a13 being corrupted by esp_rom_usb_dw_isr_handler(). The wrapper function
 * creates an additional call frame, which triggers the register window mechanism to allocate
 * a different register window, preventing the corruption from affecting tinfl_decompress().
 *
 * Alternative solutions:
 * - Add a 25 ms delay before calling tinfl_decompress() to allow esp_rom_usb_dw_isr_handler()
 *   to complete execution
 * - Temporarily disable the USB interrupt during tinfl_decompress() execution
 */
static void usb_dw_isr_handler_wrapper(void *arg)
{
    esp_rom_usb_dw_isr_handler(arg);
}

void stub_target_usb_otg_rominit_intr_attach(int intr_num, void *callback)
{
    // Route interrupt
    WRITE_PERI_REG(INTERRUPT_CORE0_USB_INTR_MAP_REG, intr_num);

    // Attach ISR with wrapper to prevent register corruption
    esp_rom_isr_attach(intr_num, usb_dw_isr_handler_wrapper, NULL);

    // Common setup
    esp_rom_cdc_acm_irq_callback_set(uart_acm_dev, callback);
    esp_rom_cdc_acm_irq_rx_enable(uart_acm_dev);
    esp_rom_cdc_acm_irq_state_enable(uart_acm_dev);

    // Enable USB global interrupt mask
    SET_PERI_REG_MASK(USB_GAHBCFG_REG, USB_GLBLLNTRMSK);

    // Enable interrupt
    esp_rom_isr_unmask(1 << intr_num);
}

void stub_target_usb_otg_handle_reset(void)
{
    // Handle any pending interrupts
    esp_rom_usb_dc_check_poll_for_interrupts();

    // Clear force download boot flag
    CLEAR_PERI_REG_MASK(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);

    // Set USB persist flags
    esp_rom_chip_usb_set_persist_flags(USBDC_PERSIST_ENA);
    esp_rom_usb_dc_prepare_persist();

    esp_rom_software_reset_cpu(0);
}
