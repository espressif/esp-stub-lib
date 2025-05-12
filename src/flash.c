/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <flash.h>

#include <log.h>
#include <target/flash.h>
#include <private/rom.h>

/* Flash geometry constants */
#define STUB_FLASH_SECTOR_SIZE          0x1000
#define STUB_FLASH_BLOCK_SIZE           0x10000
#define STUB_FLASH_PAGE_SIZE            0x100
#define STUB_FLASH_STATUS_MASK          0xFFFF

/* ROM */

extern int esp_rom_spiflash_config_param(uint32_t device_id,
                                         uint32_t chip_size, uint32_t block_size, uint32_t sector_size,
                                         uint32_t page_size, uint32_t status_mask);

static uint32_t device_id_to_flash_size(uint32_t device_id)
{
    // TODO: remove dev tracing
    STUB_LOG_TRACEF("device_id: 0x%x\n", device_id);

    const uint32_t id = device_id & 0xff;
    switch (id) {
    case 0x12:
        return 256 * 1024;
    case 0x13:
        return 512 * 1024;
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x17:
    case 0x18:
    case 0x19:
    case 0x20:
    case 0x21:
    case 0x22:
        return (id + 1 - 0x14) * 1024 * 1024;
    case 0x39:
        return 32 * 1024 * 1024;
    }

    STUB_LOGE("Unknown device_id! 0x%x", device_id);
    return 0;
}

void stub_lib_flash_init(void **state)
{
    STUB_LOG_TRACE();
    // TODO: check if we don't need the first initialization?
    // how we can check - only via EXTMEM_ICACHE_ENABLE?
    stub_target_flash_init(state);
    // TODO: check if we should use "flash_id" instead of "device_id" (see the IDF's bootloader_flash and the esptool's legacy stub)
    const uint32_t device_id = stub_target_flash_device_id();
    const uint32_t flash_size = device_id_to_flash_size(device_id);
    STUB_LOG_TRACEF("Flash size: %d MB\n", flash_size / (1024 * 1024));
    esp_rom_spiflash_config_param(device_id,
                                  flash_size,
                                  STUB_FLASH_BLOCK_SIZE,
                                  STUB_FLASH_SECTOR_SIZE,
                                  STUB_FLASH_PAGE_SIZE,
                                  STUB_FLASH_STATUS_MASK);
    STUB_LOG_TRACEF("config_param has been set\n");
    stub_target_flash_unlock(); // unlock the write protection
}

void stub_lib_flash_deinit(const void *state)
{
    stub_target_flash_deinit(state);
}

void stub_lib_flash_get_info(stub_lib_flash_info_t *info)
{
    const esp_rom_spiflash_chip_t * chip = stub_target_flash_get_rom_flashchip();

    info->id = chip->device_id;
    info->size = chip->chip_size;
    info->block_size = chip->block_size;
    info->sector_size = chip->sector_size;
    info->page_size = chip->page_size;
    info->mode = 0; // TODO: Implement
    info->encrypted = 0; // TODO: Implement
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
