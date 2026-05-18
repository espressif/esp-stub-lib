/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <esp-stub-lib/err.h>

#include <target/sdio.h>

bool __attribute__((weak)) stub_target_sdio_is_active(void)
{
    return false;
}

void __attribute__((weak)) stub_target_sdio_init(void)
{
}

bool __attribute__((weak)) stub_target_sdio_take_rx_frame(size_t *out_len)
{
    (void)out_len;
    return false;
}

int __attribute__((weak)) stub_target_sdio_rearm(uint8_t *buf, size_t max_size)
{
    (void)buf;
    (void)max_size;
    return STUB_LIB_ERR_NOT_SUPPORTED;
}

int __attribute__((weak)) stub_target_sdio_tx_frame(const void *data, size_t len)
{
    (void)data;
    (void)len;
    return STUB_LIB_ERR_NOT_SUPPORTED;
}
