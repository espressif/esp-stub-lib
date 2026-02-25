/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>

#include <esp-stub-lib/log.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>
#include <target/flash.h>

#include <soc/spi_mem_compat.h>

void stub_target_spi_wait_ready(void)
{
    while (REG_GET_FIELD(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_MST_ST) ||
           REG_GET_FIELD(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_SLV_ST)) {
        /* busy wait */
    }
}

bool stub_target_flash_needs_attach(void)
{
    return !stub_target_cache_is_enabled();
}

void stub_target_flash_init(void *state, stub_lib_flash_attach_policy_t attach_policy)
{
    (void)state;

    if (attach_policy == STUB_LIB_FLASH_ATTACH_ALWAYS || stub_target_flash_needs_attach()) {
        STUB_LOGD("Attach spi flash...\n");
        stub_target_flash_attach(0, 0);
    }

    REG_SET_BIT(SPI_MEM_USER_REG(1), SPI_MEM_USR_COMMAND);
}
