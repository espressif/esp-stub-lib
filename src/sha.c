/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <target/sha.h>

void stub_lib_sha256_start(void)
{
    stub_target_sha256_start();
}

void stub_lib_sha256_data(const void *data, uint32_t data_len)
{
    stub_target_sha256_data(data, data_len);
}

void stub_lib_sha256_finish(uint8_t *digest)
{
    stub_target_sha256_finish(digest);
}
