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
#endif

// Forward declaration
typedef void cdc_acm_device;

/**
 * @brief USB CDC chip-specific configuration structure
 *
 * This structure encapsulates all chip-specific parameters needed for USB CDC
 * initialization and operation. Each target provides an instance of this structure
 * with values appropriate for that chip.
 */
typedef struct {
    // Register addresses
    uint32_t usb_base_reg;              // USB peripheral base address
    uint32_t rtc_base_reg;               // RTC controller base address
} usb_cdc_chip_config_t;

/**
 * @brief Initialize common USB CDC state
 *
 * @param rx_callback Callback function to be called when data is received
 */
void usb_cdc_common_init_state(void (*rx_callback)(uint8_t));

/**
 * @brief Common CDC ACM callback handler
 *
 * This function handles RX data and line state changes. It should be registered
 * with cdc_acm_irq_callback_set().
 *
 * @param dev CDC ACM device pointer
 * @param status Status code (ACM_STATUS_RX or ACM_STATUS_LINESTATE_CHANGED)
 */
void usb_cdc_common_callback(cdc_acm_device *dev, int status);

/**
 * @brief Flush transmit buffer to USB device
 */
void usb_cdc_common_flush(void);

/**
 * @brief Transmit a single character with buffering
 *
 * Characters are buffered and automatically flushed when:
 * - The buffer is full (64 bytes)
 * - A SLIP END character (0xC0) is encountered
 *
 * @param c Character to transmit
 * @return 0 if successful, non-zero if error occurred
 */
uint8_t usb_cdc_common_tx_one_char(uint8_t c);

/**
 * @brief Flush transmit buffer
 *
 * Wrapper around usb_cdc_common_flush() for API consistency.
 */
void usb_cdc_common_tx_flush(void);

/**
 * @brief Receive a single character (blocking)
 *
 * This function blocks until data is available in the RX FIFO.
 *
 * @return Received character
 */
uint8_t usb_cdc_common_rx_one_char(void);

/**
 * @brief Enable or disable async RX interrupts
 *
 * @param enable true to enable, false to disable
 */
void usb_cdc_common_rx_async_enable(bool enable);

/**
 * @brief Check if reset was requested via RTS line
 *
 * @return true if reset was requested, false otherwise
 */
bool usb_cdc_common_is_reset_requested(void);

/**
 * @brief Attach and enable USB interrupt handler
 *
 * This function attaches the ISR handler and enables the interrupt.
 * Chip-specific interrupt routing should be done before calling this function.
 *
 * @param int_num USB interrupt number
 * @param isr_handler ISR handler function (or wrapper if needed)
 */
void usb_cdc_common_attach_isr(uint8_t int_num, void (*isr_handler)(void *arg));

/**
 * @brief Handle reset request with chip-specific register addresses
 *
 * This function performs the complete reset sequence:
 * - Clears reset flag
 * - Masks interrupts
 * - Waits for USB to settle
 * - Polls pending interrupts
 * - Clears RTC force download flag
 * - Sets USB persist flags
 * - Performs software reset
 *
 * @param config Chip-specific configuration structure
 */
void usb_cdc_common_handle_reset(const usb_cdc_chip_config_t *config);

#ifdef __cplusplus
}
#endif
