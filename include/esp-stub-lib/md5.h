/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#pragma once

#include <stdint.h>

struct stub_lib_md5_ctx {
    uint32_t total[4];
    uint32_t state[2];
    uint8_t buffer[64];
};

/**
 * @brief Initialize an MD5 context.
 *
 * @param ctx Pointer to the MD5 context.
 */
void stub_lib_md5_init(void *ctx);

/**
 * @brief Update an MD5 context with additional data.
 *
 * @param ctx Pointer to the MD5 context.
 * @param data Pointer to the data to update the context with.
 * @param len Number of bytes to process.
 */
void stub_lib_md5_update(void *ctx, const uint8_t *data, uint32_t len);

/**
 * @brief Finalize an MD5 context and compute the digest.
 *
 * @param ctx Pointer to the MD5 context.
 * @param digest Pointer to the buffer to store the digest.
 */
void stub_lib_md5_final(void *ctx, uint8_t digest[16]);
