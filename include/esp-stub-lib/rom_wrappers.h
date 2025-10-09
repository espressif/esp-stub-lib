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
void stub_lib_delay_us(uint32_t us);

/**
 * @brief Compute little-endian CRC16 over a byte buffer.
 *
 * @param crc Initial CRC value (seed).
 * @param buf Pointer to input bytes. Must be non-NULL if len > 0.
 * @param len Number of bytes to process.
 * @return Updated CRC16 value.
 */
uint16_t stub_lib_crc16_le(uint16_t crc, const uint8_t *buf, uint32_t len);

#ifdef __cplusplus
}
#endif // __cplusplus
