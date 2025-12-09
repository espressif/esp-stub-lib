/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Check if USB CDC is supported on this chip
 *
 * @return true if USB CDC is supported, false otherwise
 */
bool stub_target_usb_cdc_is_supported(void);

/**
 * @brief Get the USB OTG number for this chip
 *
 * This is the value that appears in UartDevice->buff_uart_no when USB CDC is active.
 *
 * @return USB OTG number, or 0 if not supported
 */
uint8_t stub_target_usb_cdc_get_otg_num(void);

/**
 * @brief Initialize USB CDC for communication
 *
 * This function sets up USB CDC interrupts and callbacks.
 *
 * @param rx_callback Callback function to be called when data is received
 */
void stub_target_usb_cdc_init(void (*rx_callback)(uint8_t));

/**
 * @brief Transmit a single byte over USB CDC
 *
 * @param c Byte to transmit
 * @return 0 if successful, non-zero if error occurred
 */
uint8_t stub_target_usb_cdc_tx_one_char(uint8_t c);

/**
 * @brief Flush any buffered USB CDC transmit data
 */
void stub_target_usb_cdc_tx_flush(void);

/**
 * @brief Receive a single byte from USB CDC (blocking)
 *
 * @return Received byte
 */
uint8_t stub_target_usb_cdc_rx_one_char(void);

/**
 * @brief Enable or disable USB CDC RX interrupts
 *
 * @param enable true to enable, false to disable
 */
void stub_target_usb_cdc_rx_async_enable(bool enable);

/**
 * @brief Check if USB CDC reset was requested via RTS line
 *
 * @return true if reset was requested, false otherwise
 */
bool stub_target_usb_cdc_is_reset_requested(void);

/**
 * @brief Handle USB CDC reset request
 *
 * This function performs the necessary cleanup and prepares for reset.
 */
void stub_target_usb_cdc_handle_reset(void);
