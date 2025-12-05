/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <target/usb_serial_jtag.h>
#include <soc/usb_serial_jtag_reg.h>
#include <soc/interrupt_matrix_reg.h>
#include <soc_utils.h>

// External ROM functions
extern void esp_rom_isr_attach(int int_num, void *handler, void *arg);
extern void esp_rom_isr_unmask(int int_num);
extern void esprv_intc_int_set_priority(int int_num, int priority);

bool stub_target_usb_serial_jtag_is_supported(void)
{
    return true;
}

void stub_target_usb_serial_jtag_rominit_intr_attach(int intr_num, void *handler, uint32_t flags)
{
    // Route USB interrupt to CPU
    WRITE_PERI_REG(INTMTX_CORE0_USB_INTR_MAP_REG, intr_num);
    esprv_intc_int_set_priority(intr_num, 1);

    // Clear pending interrupt flags
    WRITE_PERI_REG(USB_SERIAL_JTAG_INT_CLR_REG, 0xFFFFFFFFU);

    esp_rom_isr_attach(intr_num, handler, NULL);

    if (flags != 0U) {
        SET_PERI_REG_MASK(USB_SERIAL_JTAG_INT_ENA_REG, flags);
    }

    esp_rom_isr_unmask(1 << intr_num);
}
