/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include "rom_flash.h"

typedef struct {
    esp_rom_spiflash_chip_t chip;
    uint8_t dummy_len_plus[3];
    uint8_t sig_matrix;
} esp_rom_spiflash_legacy_data_t;

typedef struct {
    uint8_t pp_addr_bit_len;
    uint8_t se_addr_bit_len;
    uint8_t be_addr_bit_len;
    uint8_t rd_addr_bit_len;
    uint32_t read_sub_len;
    uint32_t write_sub_len;
    void* unlock;
    void* erase_sector;
    void* erase_block;
    void* read;
    void* write;
    void* encrypt_write;
    void* check_sus;
    void* wren;
    void* wait_idle;
    void* erase_area;
} esp_rom_spiflash_legacy_funcs_t;

extern esp_rom_spiflash_legacy_data_t *rom_spiflash_legacy_data;
extern esp_rom_spiflash_legacy_funcs_t *rom_spiflash_legacy_funcs;

#define g_rom_flashchip (rom_spiflash_legacy_data->chip)
#define g_rom_spiflash_dummy_len_plus (rom_spiflash_legacy_data->dummy_len_plus)
