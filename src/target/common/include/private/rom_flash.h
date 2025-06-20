/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
  * @brief SPI Flash init. CONFIG SPI is unsupported
  *
  * @param spiconfig: Deprecated compatibility API value, must be 0
  *
  * @param legacy: Deprecated compatibility API value, must be false
  *
  */
extern void esp_rom_spiflash_attach(uint32_t spiconfig, bool legacy);

/**
  * @brief Initialize flash_id from SPI_MEM_FLASH_RDID for ROM's flash config data
  *
  */
extern void esp_rom_spi_flash_update_id(void);

extern int esp_rom_spiflash_config_param(uint32_t flash_id, uint32_t chip_size,
                                         uint32_t block_size, uint32_t sector_size,
                                         uint32_t page_size, uint32_t status_mask);
