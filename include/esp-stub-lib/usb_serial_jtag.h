/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#define USB_SERIAL_JTAG_OUT_RECV_PKT_INT_ENA    (0x1 << 2)

/**
 * @brief Check if USB-Serial/JTAG is currently being used for communication
 *
 * This function checks if the ROM has initialized USB-Serial/JTAG as the communication
 * interface instead of UART.
 *
 * @return true if USB-Serial/JTAG is active, false otherwise
 */
bool stub_lib_usb_serial_jtag_is_active(void);

/**
 * @brief Attach interrupt handler to USB-Serial/JTAG and configure interrupts
 *
 * @param intr_num CPU interrupt source
 * @param handler Interrupt handler function pointer
 * @param flags Interrupt enable flags (bitwise OR of UART_INTR_* values)
 */
void stub_lib_usb_serial_jtag_rominit_intr_attach(int intr_num, void *handler, uint32_t flags);

/**
 * @brief Get and clear USB-Serial/JTAG interrupt status
 *
 * @return Bitmask of active interrupts that were cleared
 */
uint32_t stub_lib_usb_serial_jtag_clear_intr_flags(void);

/**
 * @brief Check if data is available in the USB-Serial/JTAG RX FIFO
 *
 * @return true if data is available, false otherwise
 */
bool stub_lib_usb_serial_jtag_is_data_available(void);

/**
 * @brief Read a single byte from the USB-Serial/JTAG RX FIFO
 *
 * @return The byte read from the FIFO
 */
uint8_t stub_lib_usb_serial_jtag_read_rxfifo_byte(void);

/**
 * @brief Transmit a single byte over USB-Serial/JTAG
 *
 * @param c Byte to transmit
 * @return 0 if successful, non-zero if error occurred
 */
uint8_t stub_lib_usb_serial_jtag_tx_one_char(uint8_t c);

/**
 * @brief Flush any buffered USB-Serial/JTAG transmit data
 */
void stub_lib_usb_serial_jtag_tx_flush(void);

#ifdef __cplusplus
}
#endif // __cplusplus
