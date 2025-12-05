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
 * @brief Get the USB-Serial/JTAG number for this chip
 *
 * This is the value that appears in UartDevice->buff_uart_no when USB-Serial/JTAG is active.
 *
 * @return USB-Serial/JTAG number, or 0 if not supported
 */
uint8_t stub_target_usb_serial_jtag_get_num(void);

/**
 * @brief Attach interrupt handler to USB-Serial/JTAG and configure interrupts
 *
 * @param intr_num CPU interrupt source
 * @param handler Interrupt handler function pointer
 * @param flags Interrupt enable flags (bitwise OR of UART_INTR_* values)
 */
void stub_target_usb_serial_jtag_rominit_intr_attach(int intr_num, void *handler, uint32_t flags);

/**
 * @brief Get and clear USB-Serial/JTAG interrupt status
 *
 * @return Bitmask of active interrupts that were cleared
 */
uint32_t stub_target_usb_serial_jtag_get_intr_flags(void);

/**
 * @brief Check if data is available in the USB-Serial/JTAG RX FIFO
 *
 * @return true if data is available, false otherwise
 */
bool stub_target_usb_serial_jtag_is_data_available(void);

/**
 * @brief Read a single byte from the USB-Serial/JTAG RX FIFO
 *
 * @return The byte read from the FIFO
 */
uint8_t stub_target_usb_serial_jtag_read_rxfifo_byte(void);
