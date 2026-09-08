/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <soc_utils.h>

#include <target/usb_serial_jtag.h>

#include <soc/usb_serial_jtag_reg.h>

// External ROM functions
extern void esp_rom_route_intr_matrix(int cpu_no, int periph_src, int cpu_intr_num);
extern void esp_rom_isr_attach(int int_num, void *handler, void *arg);
extern void esp_rom_isr_unmask(int int_num);
extern void esp_rom_esprv_intc_int_set_priority(int int_num, int priority);

/* Interrupt matrix source index of the USB Serial/JTAG peripheral. Must stay in sync with hardware. */
#define ETS_USB_SERIAL_JTAG_INTR_SOURCE 60

void stub_target_usb_serial_jtag_rominit_intr_attach(int intr_num, void *handler, uint32_t flags)
{
    /* Route USB interrupt to CPU. The ROM adds CLIC_EXT_INTR_NUM_OFFSET to intr_num itself,
     * so the external interrupt number is passed unmodified. */
    esp_rom_route_intr_matrix(0, ETS_USB_SERIAL_JTAG_INTR_SOURCE, intr_num);
    esp_rom_esprv_intc_int_set_priority(intr_num, 1);

    // Clear pending interrupt flags
    WRITE_PERI_REG(USB_SERIAL_JTAG_INT_CLR_REG, 0xFFFFFFFFU);

    esp_rom_isr_attach(intr_num, handler, NULL);

    if (flags != 0U) {
        SET_PERI_REG_MASK(USB_SERIAL_JTAG_INT_ENA_REG, flags);
    }

    esp_rom_isr_unmask(1 << intr_num);
}
