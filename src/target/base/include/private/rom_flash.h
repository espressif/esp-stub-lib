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

typedef enum {
    ESP_ROM_SPIFLASH_RESULT_OK,
    ESP_ROM_SPIFLASH_RESULT_ERR,
    ESP_ROM_SPIFLASH_RESULT_TIMEOUT
} esp_rom_spiflash_result_t;

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

/**
  * @brief Read data from Flash
  *
  * @param src_addr Address to read from. Should be 4 bytes aligned.
  * @param buffer Destination buffer
  * @param size Number of bytes to read. Should be 4 bytes aligned.
  *
  * @return Result
  * - ESP_ROM_SPIFLASH_RESULT_OK
  * - ESP_ROM_SPIFLASH_RESULT_ERR
  */
esp_rom_spiflash_result_t esp_rom_spiflash_read(uint32_t src_addr, uint32_t *dest, int32_t len);

/**
  * @brief Unlock SPI Flash
  *
  * @return Result
  * - ESP_ROM_SPIFLASH_RESULT_OK
  * - ESP_ROM_SPIFLASH_RESULT_ERR
  */
esp_rom_spiflash_result_t esp_rom_spiflash_unlock(void);

/**
 * @brief Check if Flash is OPI.
 *
 * @return true if eFuse indicates an OPI flash is attached.
 */
bool ets_efuse_flash_octal_mode(void);

/**
 * @brief Wait for SPI flash to be idle
 */
void esp_rom_spiflash_wait_idle(void);

/**
 * @brief Erase a sector using OPI flash interface (ESP32-S3)
 *
 * @param sector_num Sector number to erase
 * @return Result code
 */
int esp_rom_opiflash_erase_sector(uint32_t sector_num);

/**
 * @brief Erase a 64KB block using OPI flash interface (ESP32-S3)
 *
 * @param block_num Block number to erase
 * @return Result code
 */
int esp_rom_opiflash_erase_block_64k(uint32_t block_num);
