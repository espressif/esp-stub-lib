/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief Check if USB CDC (OTG) is currently being used for communication
 *
 * This function checks if the ROM has initialized USB CDC as the communication
 * interface instead of UART.
 *
 * @return true if USB CDC is active, false otherwise
 */
bool stub_lib_usb_cdc_is_active(void);

/**
 * @brief Initialize USB CDC for communication
 *
 * This function sets up USB CDC interrupts and callbacks for RX/TX.
 * It should be called after ROM initialization if USB CDC is detected.
 *
 * @param rx_callback Callback function to be called when data is received
 *                    The callback receives a single byte as parameter
 */
void stub_lib_usb_cdc_init(void (*rx_callback)(uint8_t));

/**
 * @brief Transmit a single byte over USB CDC
 *
 * @param c Byte to transmit
 * @return 0 if successful, non-zero if error occurred
 */
uint8_t stub_lib_usb_cdc_tx_one_char(uint8_t c);

/**
 * @brief Flush any buffered USB CDC transmit data
 */
void stub_lib_usb_cdc_tx_flush(void);

/**
 * @brief Receive a single byte from USB CDC (blocking)
 *
 * @note This function blocks until a byte is available.
 *       Use stub_lib_usb_cdc_init() with a callback for non-blocking operation.
 *
 * @return Received byte
 */
uint8_t stub_lib_usb_cdc_rx_one_char(void);

/**
 * @brief Enable or disable USB CDC RX interrupts
 *
 * @param enable true to enable, false to disable
 */
void stub_lib_usb_cdc_rx_async_enable(bool enable);

/**
 * @brief Check if USB CDC reset was requested via RTS line
 *
 * This function checks if the host has requested a reset by toggling RTS.
 * If reset is requested, the function returns true and should be followed
 * by calling stub_lib_usb_cdc_handle_reset().
 *
 * @return true if reset was requested, false otherwise
 */
bool stub_lib_usb_cdc_is_reset_requested(void);

/**
 * @brief Handle USB CDC reset request
 *
 * This function performs the necessary cleanup and prepares for reset.
 * It should be called when stub_lib_usb_cdc_is_reset_requested() returns true.
 */
void stub_lib_usb_cdc_handle_reset(void);

#ifdef __cplusplus
}
#endif // __cplusplus
