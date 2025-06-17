/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>

typedef struct esp_rom_spiflash_chip {
    uint32_t flash_id;
    uint32_t chip_size;    // chip size in bytes
    uint32_t block_size;
    uint32_t sector_size;
    uint32_t page_size;
    uint32_t status_mask;
} esp_rom_spiflash_chip_t;

void stub_target_flash_init(void);
uint32_t stub_target_flash_get_flash_id(void);
const esp_rom_spiflash_chip_t * stub_target_flash_get_config(void);
