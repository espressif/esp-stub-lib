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
 * @brief Read data from flash through the cache via an MMU mapping.
 *
 * Always maps the requested range into DROM and copies from the mapped
 * region. Required for encrypted flash (cache decrypts on read); on
 * non-encrypted flash callers may prefer stub_lib_flash_read_buff().
 *
 * Cache and MMU must already be initialized (see stub_lib_cache_init()).
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
