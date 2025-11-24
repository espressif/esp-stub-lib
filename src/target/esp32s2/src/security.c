/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stddef.h>
#include <stdint.h>
#include <target/security.h>

#define SECURITY_INFO_BYTES 12

uint32_t stub_target_security_info_size(void)
{
    return SECURITY_INFO_BYTES;
}
