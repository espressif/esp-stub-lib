/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <stdint.h>
#include <target/md5.h>

void stub_lib_md5_init(void *ctx)
{
    stub_target_md5_init(ctx);
}

void stub_lib_md5_update(void *ctx, const uint8_t *data, uint32_t len)
{
    stub_target_md5_update(ctx, data, len);
}

void stub_lib_md5_final(void *ctx, uint8_t digest[16])
{
    stub_target_md5_final(ctx, digest);
}
