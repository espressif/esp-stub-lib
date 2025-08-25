/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct esp_rom_spiflash_chip {
    uint32_t flash_id;
    uint32_t chip_size;    // chip size in bytes
    uint32_t block_size;
    uint32_t sector_size;
    uint32_t page_size;
    uint32_t status_mask;
} esp_rom_spiflash_chip_t;

/**
  * @brief SPI Flash init
  *
  * @param spiconfig
  * (for ESP32 chip)
  * - 0 for default SPI pins
  * - 1 for default HSPI pins
  * - other for custom pin configuration
  *
  * (for S2, C3, S3 chips)
  * - 0 for auto SPI configuration from eFuse is supported
  * - value for custom pin configuration
  *
  * (others chips)
  * - 0 is only a compatibility value, auto SPI configuration is not supported
  *
  * @param legacy
  * - false is only a deprecated compatibility API value, for all chips
  *
  */
extern void esp_rom_spiflash_attach(uint32_t spiconfig, bool legacy);

/**
  * @brief Initialize internal ROM config's flash_id from hw registers
  *
  */
extern void esp_rom_spi_flash_update_id(void);

/**
  * @brief Initialize internal ROM config from arguments
  *
  * @return Always returns 0.
  */
extern int esp_rom_spiflash_config_param(uint32_t flash_id, uint32_t chip_size,
                                         uint32_t block_size, uint32_t sector_size,
                                         uint32_t page_size, uint32_t status_mask);
