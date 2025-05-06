/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <common/target_rom.h>

typedef struct {
    esp_rom_spiflash_chip_t chip;
    uint8_t dummy_len_plus[3];
    uint8_t sig_matrix;
} esp_rom_spiflash_legacy_data_t;

/* ROM's flash config data */
extern esp_rom_spiflash_legacy_data_t *rom_spiflash_legacy_data;

const esp_rom_spiflash_chip_t *stub_target_rom_get_flash_config()
{
    return &rom_spiflash_legacy_data->chip;
}
