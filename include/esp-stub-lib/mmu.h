/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief Read data from flash, automatically handling encrypted flash.
 *
 * When flash encryption is enabled, reads through the cache via MMU mapping
 * to get decrypted data. Otherwise, reads directly via SPI.
 *
 * @param addr   Flash physical address.
 * @param buffer Destination buffer.
 * @param size   Number of bytes to read.
 *
 * @return STUB_LIB_OK on success, or a negative error code.
 */
int stub_lib_mmu_read_flash(uint32_t addr, void *buffer, uint32_t size);

#ifdef __cplusplus
}
#endif // __cplusplus
