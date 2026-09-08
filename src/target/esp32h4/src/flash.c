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

#include <private/rom_flash.h>

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

uint32_t stub_target_get_max_supported_flash_size(void)
{
    /* ESP32-H4 supports up to 32MB with 4-byte addressing */
    return MIB(32);
}

void stub_target_opiflash_exec_cmd(const opiflash_cmd_params_t *params)
{
    rom_spi_usr_cmd_legacy_funcs->exec_flash_cmd(params->spi_num,
                                                 params->mode,
                                                 params->cmd,
                                                 params->cmd_bit_len,
                                                 params->addr,
                                                 params->addr_bit_len,
                                                 params->dummy_bits,
                                                 params->mosi_data,
                                                 params->mosi_bit_len,
                                                 params->miso_data,
                                                 params->miso_bit_len,
                                                 params->cs_mask,
                                                 params->is_write_erase_operation);
}
