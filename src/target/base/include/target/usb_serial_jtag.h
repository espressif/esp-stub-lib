/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Check if USB-Serial/JTAG is supported on this chip
 *
 * @return true if USB-Serial/JTAG is supported, false otherwise
 */
bool stub_target_usb_serial_jtag_is_supported(void);

/**
 * @brief Attach interrupt handler to USB-Serial/JTAG and configure interrupts
 *
 * @param intr_num CPU interrupt source
 * @param handler Interrupt handler function pointer
 * @param flags Interrupt enable flags (bitwise OR of UART_INTR_* values)
 */
void stub_target_usb_serial_jtag_rominit_intr_attach(int intr_num, void *handler, uint32_t flags);
