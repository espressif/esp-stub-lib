/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stddef.h>
#include <stdint.h>
#include <target/security.h>
#include <err.h>

/* GetSecurityInfoProc functions from ROM */
extern uint32_t GetSecurityInfoProcEco1(int *pMsg, int *pnErr, uint8_t *buf);
extern uint32_t GetSecurityInfoProcEco7(int *pMsg, int *pnErr, uint8_t *buf);
/* ROM constant to define ECO version - address 0x40000014 */
extern uint32_t _rom_eco_version;

#define SECURITY_INFO_BYTES 20

uint32_t stub_target_security_info_size(void)
{
    return SECURITY_INFO_BYTES;
}

int stub_target_get_security_info(uint8_t *buffer, uint32_t buffer_size)
{
    if (buffer == NULL) {
        return STUB_LIB_ERR_INVALID_ARG;
    }

    uint32_t required_size = stub_target_security_info_size();
    if (buffer_size < required_size) {
        return STUB_LIB_ERR_INVALID_ARG;
    }

    uint32_t ret = 0;
    if (_rom_eco_version >= 7) {
        ret = GetSecurityInfoProcEco7(NULL, NULL, buffer);
    } else {
        ret = GetSecurityInfoProcEco1(NULL, NULL, buffer);
    }

    if (ret == 0) {
        return STUB_LIB_OK;
    }

    return STUB_LIB_FAIL;
}
