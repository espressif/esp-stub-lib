/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <target/aes_xts.h>
#include <esp-stub-lib/err.h>

void __attribute__((weak)) stub_target_aes_xts_init(void)
{
    // Empty weak function for targets that does not support AES-XTS
}

void __attribute__((weak)) stub_target_aes_xts_encrypt_trigger(
    uint32_t flash_addr,
    void *data,
    uint32_t block_size)
{
    // Empty weak function for targets that does not support AES-XTS
    (void)flash_addr;
    (void)data;
    (void)block_size;
}

int __attribute__((weak)) stub_target_aes_xts_wait_data_ready(uint32_t timeout_us)
{
    // Empty weak function for targets that does not support AES-XTS
    (void)timeout_us;
    return STUB_LIB_ERR_NOT_SUPPORTED;
}

void __attribute__((weak)) stub_target_aes_xts_destroy(void)
{
    // Empty weak function for targets that does not support AES-XTS
}
