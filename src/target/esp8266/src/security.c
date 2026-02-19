/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <target/security.h>
#include <err.h>

/* ESP8266 does not support GetSecurityInfoProc */
uint32_t stub_target_security_info_size(void)
{
    return 0;
}

int stub_target_get_security_info(uint8_t *buffer, uint32_t buffer_size)
{
    (void)buffer;
    (void)buffer_size;
    return STUB_LIB_ERR_NOT_SUPPORTED;
}

bool stub_target_security_flash_is_encrypted(void)
{
    return false;
}
