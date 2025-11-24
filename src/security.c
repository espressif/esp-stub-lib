/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stddef.h>
#include <esp-stub-lib/security.h>
#include <esp-stub-lib/err.h>
#include <target/security.h>

uint32_t stub_lib_security_info_size(void)
{
    return stub_target_security_info_size();
}

int stub_lib_get_security_info(uint8_t *buffer, uint32_t buffer_size)
{
    return stub_target_get_security_info(buffer, buffer_size);
}
