/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <flash.h>
#include <err.h>
#include <log.h>
#include <bit_utils.h>
#include <target/flash.h>
#include <private/rom_flash_config.h>

stub_lib_err_t stub_lib_flash_init(void **state)
{
    stub_target_flash_init(state);
    uint32_t flash_id = stub_target_flash_get_flash_id();
    uint32_t flash_size = stub_target_flash_id_to_flash_size(flash_id);
    if (flash_size == 0) {
        STUB_LOGE("Invalid flash size: 0\n");
        return STUB_LIB_ERR_UNKNOWN_FLASH_ID;
    }
    STUB_LOG_TRACEF("Flash size: %d MB\n", MB(flash_size));

    stub_target_flash_update_config(flash_id, flash_size);

    return STUB_LIB_OK;
}

void stub_lib_flash_deinit(const void *state)
{
    stub_target_flash_deinit(state);
}

void stub_lib_flash_get_info(stub_lib_flash_info_t *info)
{
    const esp_rom_spiflash_chip_t * chip = stub_target_flash_get_config();

    info->id = chip->flash_id;
    info->size = chip->chip_size;
    info->block_size = chip->block_size;
    info->sector_size = chip->sector_size;
    info->page_size = chip->page_size;
    info->mode = 0; // TODO: Implement
    info->encrypted = 0; // TODO: Implement
}

void stub_lib_flash_info_print(const stub_lib_flash_info_t *info)
{
    (void)info; // to avoid the 'unused-parameter' warning when LOG is disabled
    STUB_LOGI("Flash info:\n"
              "\tid: 0x%x, size: %d KB,\n"
              "\tblock: %d KB (0x%x), sector: %d B (0x%x), page: %d B (0x%x),\n"
              "\tmode: %d, enc: %d\n",
              info->id,
              KB(info->size),
              KB(info->block_size), info->block_size,
              info->sector_size, info->sector_size,
              info->page_size, info->page_size,
              info->mode,
              info->encrypted
             );
}
