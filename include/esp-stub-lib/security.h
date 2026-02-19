/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief Get security information buffer size for the current chip.
 *
 * @return Size in bytes:
 * - 12 for ESP32S2
 * - 20 for ESP32S3 and later chips
 * - 0 if not supported on this chip
 */
uint32_t stub_lib_security_info_size(void);

/**
 * @brief Get security information from the chip.
 *
 * @param[out] buffer Buffer to store security info. Must be at least stub_lib_security_info_size() bytes.
 * @param[in] buffer_size Size of the buffer in bytes.
 *
 * @return Error code:
 * - STUB_LIB_OK if success
 * - STUB_LIB_ERR_NOT_SUPPORTED if security info is not supported on this chip
 * - STUB_LIB_ERR_INVALID_ARG if buffer is NULL or buffer_size is too small
 * - STUB_LIB_FAIL if the operation failed
 */
int stub_lib_get_security_info(uint8_t *buffer, uint32_t buffer_size);

/**
 * @brief Check whether flash encryption is enabled.
 *
 * @return true if flash encryption is enabled, false otherwise.
 */
bool stub_lib_security_flash_is_encrypted(void);

#ifdef __cplusplus
}
#endif // __cplusplus
