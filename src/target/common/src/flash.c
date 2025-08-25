/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <log.h>
#include <target/flash.h>
#include <private/rom_flash.h>

/* Flash geometry constants */
#define STUB_FLASH_SECTOR_SIZE          0x1000
#define STUB_FLASH_BLOCK_SIZE           0x10000
#define STUB_FLASH_PAGE_SIZE            0x100
#define STUB_FLASH_STATUS_MASK          0xFFFF

void stub_target_flash_update_config(uint32_t flash_id, uint32_t flash_size)
{
    (void)esp_rom_spiflash_config_param(flash_id,
                                        flash_size,
                                        STUB_FLASH_BLOCK_SIZE,
                                        STUB_FLASH_SECTOR_SIZE,
                                        STUB_FLASH_PAGE_SIZE,
                                        STUB_FLASH_STATUS_MASK);
}

uint32_t stub_target_flash_id_to_flash_size(uint32_t flash_id)
{
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

    STUB_LOGE("Unknown flash_id: 0x%x", flash_id);
    return 0;
}
