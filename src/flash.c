/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <flash.h>
#include <err.h>
#include <log.h>
#include <target/flash.h>

/* Flash geometry constants */
#define STUB_FLASH_SECTOR_SIZE          0x1000
#define STUB_FLASH_BLOCK_SIZE           0x10000
#define STUB_FLASH_PAGE_SIZE            0x100
#define STUB_FLASH_STATUS_MASK          0xFFFF

extern int esp_rom_spiflash_config_param(uint32_t flash_id, uint32_t chip_size,
                                         uint32_t block_size, uint32_t sector_size,
                                         uint32_t page_size, uint32_t status_mask);

static uint32_t flash_id_to_flash_size(uint32_t flash_id)
{
    // TODO: remove dev tracing
    STUB_LOG_TRACEF("flash_id: 0x%x\n", flash_id);

    const uint32_t id = flash_id & 0xff;
    switch (id) {
    case 0x12:  // 256 KB
    case 0x13:
    case 0x14:  // 1 MB
    case 0x15:
    case 0x16:
    case 0x17:
    case 0x18:
    case 0x19:  // 32 MB
    case 0x1A:  // 64 MB
    case 0x1B:  // 128 MB
    case 0x1C:  // 256 MB
        return 1u << id;
    case 0x39:
        return 32 * 1024 * 1024;
    }

    STUB_LOGE("Unknown flash_id! 0x%x", flash_id);
    return 0;
}

static void flash_update_config(uint32_t flash_id, uint32_t flash_size)
{
    esp_rom_spiflash_config_param(flash_id,
                                  flash_size,
                                  STUB_FLASH_BLOCK_SIZE,
                                  STUB_FLASH_SECTOR_SIZE,
                                  STUB_FLASH_PAGE_SIZE,
                                  STUB_FLASH_STATUS_MASK);
}

stub_lib_err_t stub_lib_flash_init(void **state)
{
    (void)state;
    STUB_LOG_TRACE();

    stub_target_flash_init();
    uint32_t flash_id = stub_target_flash_get_flash_id();
    uint32_t flash_size = flash_id_to_flash_size(flash_id);
    if (flash_size == 0) {
        STUB_LOGE("Invalid flash size: 0\n");
        return STUB_LIB_FAIL;
    }
    STUB_LOG_TRACEF("Flash size: %d MB\n", flash_size / (1024 * 1024));

    flash_update_config(flash_id, flash_size);
    STUB_LOG_TRACEF("config_param has been set\n");

    return STUB_LIB_OK;
}

void stub_lib_flash_deinit(const void *state)
{
    (void)state;
}

stub_lib_err_t stub_lib_flash_get_info(stub_lib_flash_info_t *info)
{
    const esp_rom_spiflash_chip_t * chip = stub_target_flash_get_config();

    info->id = chip->flash_id;
    info->size = chip->chip_size;
    info->block_size = chip->block_size;
    info->sector_size = chip->sector_size;
    info->page_size = chip->page_size;
    info->mode = 0; // TODO: Implement
    info->encrypted = 0; // TODO: Implement

    return STUB_LIB_OK;
}

void stub_lib_flash_info_print(const stub_lib_flash_info_t *info)
{
    (void)info;
    STUB_LOGI("Flash info:\n"
              "\tid: 0x%x, size: %d KB,\n"
              "\tblock: %d KB (0x%x), sector: %d B (0x%x), page: %d B (0x%x),\n"
              "\tmode: %d, enc: %d\n",
              info->id,
              info->size / 1024,
              info->block_size / 1024, info->block_size,
              info->sector_size, info->sector_size,
              info->page_size, info->page_size,
              info->mode,
              info->encrypted
             );
}
