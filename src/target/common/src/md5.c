/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <stdint.h>

extern void MD5Init(void *ctx);
extern void MD5Update(void *ctx, const uint8_t *data, uint32_t len);
extern void MD5Final(uint8_t digest[16], void *ctx);

void __attribute__((weak)) stub_target_md5_init(void *ctx)
{
    MD5Init(ctx);
}

void __attribute__((weak)) stub_target_md5_update(void *ctx, const uint8_t *data, uint32_t len)
{
    MD5Update(ctx, data, len);
}

void __attribute__((weak)) stub_target_md5_final(void *ctx, uint8_t digest[16])
{
    MD5Final(digest, ctx);
}
