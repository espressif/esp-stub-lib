/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stddef.h>
#include <stdint.h>
#include <target/security.h>
#include <err.h>

/* GetSecurityInfoProc function from ROM */
extern uint32_t GetSecurityInfoProc(int* pMsg, int* pnErr, uint8_t *buf);

#define SECURITY_INFO_BYTES_DEFAULT 20

uint32_t __attribute__((weak)) stub_target_security_info_size(void)
{
    return SECURITY_INFO_BYTES_DEFAULT;
}

int __attribute__((weak)) stub_target_get_security_info(uint8_t *buffer, uint32_t buffer_size)
{
    if (buffer == NULL) {
        return STUB_LIB_ERR_INVALID_ARG;
    }

    uint32_t required_size = stub_target_security_info_size();
    if (buffer_size < required_size) {
        return STUB_LIB_ERR_INVALID_ARG;
    }

    uint32_t ret = GetSecurityInfoProc(NULL, NULL, buffer);
    if (ret == 0) {
        return STUB_LIB_OK;
    }

    return STUB_LIB_FAIL;
}
