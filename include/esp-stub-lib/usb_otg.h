/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief Check if USB-OTG is currently being used for communication
 *
 * This function checks if the ROM has initialized USB-OTG as the communication
 * interface instead of UART.
 *
 * @return true if USB-OTG is active, false otherwise
 */
bool stub_lib_usb_otg_is_active(void);

/**
 * @brief Initialize USB-OTG for communication
 *
 * This function sets up USB-OTG interrupts and the RX callback function.
 * It should be called after ROM initialization if USB-OTG is detected.
 *
 * @param intr_num Interrupt number
 * @param rx_callback Callback function to be called when data is received
 *                    The callback receives a single byte as parameter
 */
void stub_lib_usb_otg_rominit_intr_attach(int intr_num, void (*rx_callback)(uint8_t));

/**
 * @brief Initialize USB-OTG for communication, with received data passed in blocks
 *
 * Same as stub_lib_usb_otg_rominit_intr_attach(), but the callback gets up to one
 * USB packet of received bytes per call instead of one byte.
 *
 * @param intr_num Interrupt number
 * @param rx_block_callback Callback function to be called with the received bytes
 */
void stub_lib_usb_otg_rominit_intr_attach_rx_block(int intr_num,
                                                   void (*rx_block_callback)(const uint8_t *data, size_t len));

/**
 * @brief Transmit a single byte over USB-OTG
 *
 * @param c Byte to transmit
 * @return 0 if successful, non-zero if error occurred
 */
uint8_t stub_lib_usb_otg_tx_one_char(uint8_t c);

/**
 * @brief Transmit bytes over USB-OTG
 *
 * Buffers the bytes like tx_one_char and sends each full buffer.
 *
 * @param data Bytes to transmit
 * @param len Number of bytes
 */
void stub_lib_usb_otg_tx_write(const uint8_t *data, size_t len);

/**
 * @brief Flush any buffered USB-OTG transmit data
 */
void stub_lib_usb_otg_tx_flush(void);

/**
 * @brief Enable or disable USB-OTG RX interrupts
 *
 * @param enable true to enable, false to disable
 */
void stub_lib_usb_otg_rx_async_enable(bool enable);

/**
 * @brief Check if USB-OTG reset was requested via RTS line
 *
 * This function checks if the host has requested a reset by toggling RTS.
 * If reset is requested, the function returns true and should be followed
 * by calling stub_lib_usb_otg_handle_reset().
 *
 * @return true if reset was requested, false otherwise
 */
bool stub_lib_usb_otg_is_reset_requested(void);

/**
 * @brief Handle USB-OTG reset request
 *
 * This function performs the necessary cleanup and prepares for reset.
 * It should be called when stub_lib_usb_otg_is_reset_requested() returns true.
 */
void stub_lib_usb_otg_handle_reset(void);

#ifdef __cplusplus
}
#endif // __cplusplus
