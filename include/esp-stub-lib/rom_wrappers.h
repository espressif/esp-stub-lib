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

struct stub_lib_md5_ctx {
    uint32_t total[2];
    uint32_t state[4];
    uint8_t buffer[64];
};

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

/**
 * @brief Initialize an MD5 context.
 *
 * @param ctx Pointer to the MD5 context.
 */
void stub_lib_md5_init(struct stub_lib_md5_ctx *ctx);

/**
 * @brief Update an MD5 context with additional data.
 *
 * @param ctx Pointer to the MD5 context.
 * @param data Pointer to the data to update the context with.
 * @param len Number of bytes to process.
 */
void stub_lib_md5_update(struct stub_lib_md5_ctx *ctx, const uint8_t *data, uint32_t len);

/**
 * @brief Finalize an MD5 context and compute the digest.
 *
 * @param ctx Pointer to the MD5 context.
 * @param digest Pointer to the buffer to store the digest.
 *
 */
void stub_lib_md5_final(struct stub_lib_md5_ctx *ctx, uint8_t digest[16]);

#ifdef __cplusplus
}
#endif // __cplusplus
