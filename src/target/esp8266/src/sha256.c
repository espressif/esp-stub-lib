/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <stdint.h>

#include <esp-stub-lib/log.h>

void stub_target_sha256_start(void)
{
    STUB_LOG_TRACEF("not implemented\n");
}

void stub_target_sha256_data(const void *data, uint32_t data_len)
{
    (void)data;
    (void)data_len;

    STUB_LOG_TRACEF("not implemented\n");
}

void stub_target_sha256_finish(uint8_t *digest)
{
    (void)digest;

    STUB_LOG_TRACEF("not implemented\n");
}
