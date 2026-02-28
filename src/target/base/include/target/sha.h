/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Initialize SHA256 hardware and prepare for hashing.
 *
 * Target-specific: enables the SHA peripheral and initializes the context.
 */
void stub_target_sha256_start(void);

/**
 * @brief Feed data into the running SHA256 hash.
 *
 * @param data Pointer to input data.
 * @param data_len Number of bytes to hash.
 */
void stub_target_sha256_data(const void *data, size_t data_len);

/**
 * @brief Finalize the SHA256 hash and retrieve the digest.
 *
 * If digest is NULL the context is cleared without producing a hash
 * and the SHA hardware is released.
 *
 * @param[out] digest Buffer of at least 32 bytes, or NULL to cancel.
 */
void stub_target_sha256_finish(uint8_t *digest);
