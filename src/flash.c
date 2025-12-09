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

/* Flash geometry constants */
#define STUB_FLASH_SECTOR_SIZE          0x1000
#define STUB_FLASH_BLOCK_SIZE           0x10000
#define STUB_FLASH_PAGE_SIZE            0x100
#define STUB_FLASH_STATUS_MASK          0xFFFF

int stub_lib_flash_update_config(stub_lib_flash_config_t *config)
{
    return stub_target_flash_update_config(config->flash_id, config->flash_size, config->block_size, config->sector_size,
                                           config->page_size, config->status_mask);
}

void stub_lib_flash_attach(uint32_t ishspi, bool legacy)
{
    stub_target_flash_attach(ishspi, legacy);
}

int stub_lib_flash_init(void **state)
{
    stub_target_flash_init(state);
    uint32_t flash_id = stub_target_flash_get_flash_id();
    uint32_t flash_size = stub_target_flash_id_to_flash_size(flash_id);
    if (flash_size == 0) {
        return STUB_LIB_ERR_UNKNOWN_FLASH_ID;
    }
    STUB_LOG_TRACEF("Flash size: %d MB\n", MB(flash_size));

    return stub_target_flash_update_config(flash_id, flash_size, STUB_FLASH_BLOCK_SIZE, STUB_FLASH_SECTOR_SIZE,
                                           STUB_FLASH_PAGE_SIZE, STUB_FLASH_STATUS_MASK);
}

void stub_lib_flash_deinit(const void *state)
{
    stub_target_flash_deinit(state);
}

void stub_lib_flash_get_info(stub_lib_flash_info_t *info)
{
    const esp_rom_spiflash_chip_t *chip = stub_target_flash_get_config();

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

int stub_lib_flash_read_buff(uint32_t addr, void *buffer, uint32_t size)
{
    return stub_target_flash_read_buff(addr, buffer, size);
}

int stub_lib_flash_write_buff(uint32_t addr, const void *buffer, uint32_t size, int encrypt)
{
    return stub_target_flash_write_buff(addr, buffer, size, encrypt);
}

int stub_lib_flash_erase_chip(void)
{
    return stub_target_flash_erase_chip();
}

int stub_lib_flash_erase_sector(uint32_t addr)
{
    return stub_target_flash_erase_sector(addr);
}

int stub_lib_flash_erase_block(uint32_t addr)
{
    return stub_target_flash_erase_block(addr);
}

int stub_lib_flash_erase_area(uint32_t addr, uint32_t size)
{
    return stub_target_flash_erase_area(addr, size);
}
