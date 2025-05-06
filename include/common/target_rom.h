/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>

/* Flash geometry constants */
#define STUB_FLASH_SECTOR_SIZE          0x1000
#define STUB_FLASH_BLOCK_SIZE           0x10000
#define STUB_FLASH_PAGE_SIZE            0x100
#define STUB_FLASH_STATUS_MASK          0xFFFF

typedef struct esp_rom_spiflash_chip {
    uint32_t device_id;
    uint32_t chip_size;    // chip size in bytes
    uint32_t block_size;
    uint32_t sector_size;
    uint32_t page_size;
    uint32_t status_mask;
} esp_rom_spiflash_chip_t;

extern int esp_rom_spiflash_config_param(uint32_t device_id, uint32_t chip_size,
                                         uint32_t block_size, uint32_t sector_size,
                                         uint32_t page_size, uint32_t status_mask);

__attribute__((always_inline)) inline static
const esp_rom_spiflash_chip_t * stub_target_rom_get_flash_config(void);

__attribute__((always_inline)) inline static
void stub_target_rom_update_flash_config(uint32_t flash_id, uint32_t flash_size)
{
    esp_rom_spiflash_config_param(flash_id,
                                  flash_size,
                                  STUB_FLASH_BLOCK_SIZE,
                                  STUB_FLASH_SECTOR_SIZE,
                                  STUB_FLASH_PAGE_SIZE,
                                  STUB_FLASH_STATUS_MASK);
}
