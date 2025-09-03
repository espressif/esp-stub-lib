/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief Busy-wait delay for the given number of microseconds.
 *
 * @param us Number of microseconds to delay.
 */
void esp_stub_lib_delay_us(uint32_t us);

/**
 * @brief Compute little-endian CRC16 over a byte buffer.
 *
 * @param crc Initial CRC value (seed).
 * @param buf Pointer to input bytes. Must be non-NULL if len > 0.
 * @param len Number of bytes to process.
 * @return Updated CRC16 value.
 */
uint16_t esp_stub_lib_crc16_le(uint16_t crc, const uint8_t *buf, uint32_t len);

/**
 * @brief Transmit a single byte over UART.
 *
 * @param c Byte to transmit.
 * @return 0 if successful, non-zero if error occurred.
 */
uint8_t esp_stub_lib_tx_one_char(uint8_t c);

/**
 * @brief Receive a single byte (blocking) from UART.
 *
 * @return Received byte.
 */
uint8_t esp_stub_lib_rx_one_char(void);

/**
 * @brief Flush any buffered transmit data.
 */
void esp_stub_lib_tx_flush(void);

#ifdef __cplusplus
}
#endif // __cplusplus
