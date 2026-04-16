/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <soc_utils.h>

#include <target/usb_otg.h>

#include <soc/lp_system_reg.h>
#include <soc/reg_base.h>
#include <soc/usb_otghs_reg.h>

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
extern void esp_rom_esprv_intc_int_set_priority(int int_num, int priority);

/*
 * Index of ETS_USB_OTGHS_INTR_SOURCE in `periph_interrupt_t`
 * (components/soc/esp32s31/include/soc/interrupts.h in ESP-IDF). Must stay in sync with hardware.
 */
#define ETS_USB_OTGHS_INTR_SOURCE 99
#define USBDC_PERSIST_ENA         (1U << 31)

bool stub_target_usb_otg_is_supported(void)
{
    return true;
}

void stub_target_usb_otg_rominit_intr_attach(int intr_num, void *callback)
{
    // Route interrupt
    esp_rom_route_intr_matrix(0, ETS_USB_OTGHS_INTR_SOURCE, intr_num);

    // Set interrupt priority
    esp_rom_esprv_intc_int_set_priority(intr_num, 1);

    // Attach ISR
    esp_rom_isr_attach(intr_num, esp_rom_usb_dw_isr_handler, NULL);

    // Common setup
    esp_rom_cdc_acm_irq_callback_set(uart_acm_dev, callback);
    esp_rom_cdc_acm_irq_rx_enable(uart_acm_dev);
    esp_rom_cdc_acm_irq_state_enable(uart_acm_dev);

    // Enable USB global interrupt mask
    SET_PERI_REG_MASK(USB_OTGHS_GAHBCFG_REG, USB_OTGHS_GAHBCFG_GLBLINTRMSK);

    // Enable interrupt
    esp_rom_isr_unmask(1 << intr_num);
}

void stub_target_usb_otg_handle_reset(void)
{
    // Handle any pending interrupts
    esp_rom_usb_dc_check_poll_for_interrupts();

    // Clear force download boot flag
    CLEAR_PERI_REG_MASK(LP_SYSTEM_REG_SYS_CTRL_REG, LP_SYSTEM_REG_FORCE_DOWNLOAD_BOOT);

    // Set USB persist flags
    esp_rom_chip_usb_set_persist_flags(USBDC_PERSIST_ENA);
    esp_rom_usb_dc_prepare_persist();

    esp_rom_software_reset_cpu(0);
}
