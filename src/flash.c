/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <flash.h>

#include <log.h>
#include <common/target_rom.h>
#include <common/target_flash.h>

#include <target/rom_impl.h> // corresponding implementation for common/target_rom.h
#include <target/flash_impl.h> // corresponding implementation for common/target_flash.h

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

void stub_lib_flash_init(void **state)
{
    (void)state;
    STUB_LOG_TRACE();
    // TODO: check if we don't need the first initialization?
    // how we can check - only via EXTMEM_ICACHE_ENABLE?
    stub_target_flash_init();
    // TODO: check if we should use "flash_id" instead of "flash_id" (see the IDF's bootloader_flash and the esptool's legacy stub)
    const uint32_t flash_id = stub_target_flash_get_flash_id();
    const uint32_t flash_size = flash_id_to_flash_size(flash_id);
    STUB_LOG_TRACEF("Flash size: %d MB\n", flash_size / (1024 * 1024));
    stub_target_rom_update_flash_config(flash_id, flash_size);
    STUB_LOG_TRACEF("config_param has been set\n");
}

void stub_lib_flash_deinit(const void *state)
{
    (void)state;
}

void stub_lib_flash_get_info(stub_lib_flash_info_t *info)
{
    const esp_rom_spiflash_chip_t * chip = stub_target_rom_get_flash_config();

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
