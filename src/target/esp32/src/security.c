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
#include <esp-stub-lib/soc_utils.h>
#include <soc/reg_base.h>

#define EFUSE_BLK0_RDATA0_REG      (DR_REG_EFUSE_BASE + 0x0)
#define EFUSE_RD_FLASH_CRYPT_CNT_V 0x0000007FU
#define EFUSE_RD_FLASH_CRYPT_CNT_S 20

/* ESP32 does not support GetSecurityInfoProc */
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
    uint32_t flash_crypt_cnt = REG_READ(EFUSE_BLK0_RDATA0_REG);
    flash_crypt_cnt = (flash_crypt_cnt >> EFUSE_RD_FLASH_CRYPT_CNT_S) & EFUSE_RD_FLASH_CRYPT_CNT_V;

    bool enabled = false;
    while (flash_crypt_cnt) {
        if (flash_crypt_cnt & 1U) {
            enabled = !enabled;
        }
        flash_crypt_cnt >>= 1;
    }

    return enabled;
}
