/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <target/sha.h>

#define SHA2_256 2

/*
 * Opaque buffer for the ROM SHA_CTX struct (not packed, 216 bytes with
 * natural alignment padding). Declared as uint32_t to match the 4-byte
 * alignment the ROM expects for its uint32_t state[] and total_bits[] fields.
 */
static uint32_t sha_ctx[54]; // 216 bytes = sizeof(SHA_CTX)

extern void ets_sha_enable(void);
extern void ets_sha_disable(void);
extern void ets_sha_init(void *ctx, int type);
extern void ets_sha_update(void *ctx, const void *data, size_t data_len, bool update_ctx);
extern void ets_sha_finish(void *ctx, uint8_t *digest);

void __attribute__((weak)) stub_target_sha256_start(void)
{
    ets_sha_enable();
    ets_sha_init(sha_ctx, SHA2_256);
}

void __attribute__((weak)) stub_target_sha256_data(const void *data, size_t data_len)
{
    ets_sha_update(sha_ctx, data, data_len, false);
}

void __attribute__((weak)) stub_target_sha256_finish(uint8_t *digest)
{
    if (!digest) {
        memset(sha_ctx, 0, sizeof(sha_ctx));
        ets_sha_disable();
        return;
    }
    ets_sha_finish(sha_ctx, digest);
    ets_sha_disable();
}
