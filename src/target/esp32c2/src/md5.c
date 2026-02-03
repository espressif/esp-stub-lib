/**
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <stdint.h>

// ESP32-C2 ROM uses mbedtls_md5 instead of MD5Init, MD5Update, MD5Final
// Context structure is a bit different, but the layout is the same.
extern int mbedtls_md5_starts_ret(void *ctx);
extern int mbedtls_md5_update_ret(void *ctx, const uint8_t *data, uint32_t len);
extern int mbedtls_md5_finish_ret(void *ctx, uint8_t digest[16]);

void stub_target_md5_init(void *ctx)
{
    mbedtls_md5_starts_ret(ctx);
}

void stub_target_md5_update(void *ctx, const uint8_t *data, uint32_t len)
{
    mbedtls_md5_update_ret(ctx, data, len);
}

void stub_target_md5_final(void *ctx, uint8_t digest[16])
{
    mbedtls_md5_finish_ret(ctx, digest);
}
