/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <stdint.h>
#include <esp-stub-lib/rom_wrappers.h>

extern void ets_delay_us(uint32_t us);
extern uint16_t crc16_le(uint16_t crc, const uint8_t *buf, uint32_t len);
extern void MD5Init(struct stub_lib_md5_ctx *ctx);
extern void MD5Update(struct stub_lib_md5_ctx *ctx, const uint8_t *data, uint32_t len);
extern void MD5Final(uint8_t digest[16], struct stub_lib_md5_ctx *ctx);

void stub_lib_delay_us(uint32_t us)
{
    ets_delay_us(us);
}

uint16_t stub_lib_crc16_le(uint16_t crc, const uint8_t *buf, uint32_t len)
{
    return crc16_le(crc, buf, len);
}

void stub_lib_md5_init(struct stub_lib_md5_ctx *ctx)
{
    MD5Init(ctx);
}

void stub_lib_md5_update(struct stub_lib_md5_ctx *ctx, const uint8_t *data, uint32_t len)
{
    MD5Update(ctx, data, len);
}

void stub_lib_md5_final(struct stub_lib_md5_ctx *ctx, uint8_t digest[16])
{
    MD5Final(digest, ctx);
}
