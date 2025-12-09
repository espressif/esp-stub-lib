/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

struct esp_rom_spiflash_chip;

/**
 * @brief Initialize SPI Flash hardware.
 *
 * Configure SPI pins, registers, mode, etc.
 *
 * @param state Unused.
 */
void stub_target_flash_init(void *state);

/**
 * @brief Not implemented. Intended for restoring the state
 *
 * @param state Unused.
 */
void stub_target_flash_deinit(const void *state);

/**
 * @brief Retrieve Flash ID (aka flash device id, aka flash chip id) from internal hw.
 *
 * @return Flash ID, that includes manufacture and size information.
 */
uint32_t stub_target_flash_get_flash_id(void);

/**
 * @brief Get a pointer to the internal SPI flash config in ROM.
 *
 * @return Always a non-NULL, but the structure may be uninitialized or incorrect.
 */
const struct esp_rom_spiflash_chip *stub_target_flash_get_config(void);

/**
 * @brief Set correct values to the internal SPI flash config in ROM
 *
 * @param flash_id Flash ID
 * @param flash_size Flash size in bytes
 * @param block_size Block size in bytes
 * @param sector_size Sector size in bytes
 * @param page_size Page size in bytes
 * @param status_mask Status mask
 *
 * @return Result:
 * - STUB_LIB_OK if success
 * - STUB_LIB_FAIL on ROM config error
 */
int stub_target_flash_update_config(uint32_t flash_id, uint32_t flash_size, uint32_t block_size, uint32_t sector_size,
                                    uint32_t page_size, uint32_t status_mask);

/**
 * @brief Infer flash size (in bytes) from Flash ID
 *
 * @param flash_id Raw Flash ID value
 *
 * @return Flash size in bytes, or:
 * - 0 if flash_id is unknown
 */
uint32_t stub_target_flash_id_to_flash_size(uint32_t flash_id);

/**
 * @brief Read data.
 *
 * Check alignment, call a ROM function.
 *
 * @param addr Address to read from. Should be 4 bytes aligned.
 * @param buffer Destination buffer
 * @param size Number of bytes to read. Should be 4 bytes aligned.
 *
 * @return Result:
 * - STUB_LIB_OK if success
 * - STUB_LIB_ERR_FLASH_READ_UNALIGNED
 * - STUB_LIB_ERR_FLASH_READ_ROM_ERR
 */
int stub_target_flash_read_buff(uint32_t addr, void *buffer, uint32_t size);

/**
 * @brief Write data to flash.
 *
 * Check alignment, call a ROM function.
 *
 * @param addr Address to write to. Should be 4 bytes aligned.
 * @param buffer Source buffer
 * @param size Number of bytes to write. Should be 4 bytes aligned.
 * @param encrypt Whether to use encrypted write
 *
 * @return Result:
 * - STUB_LIB_OK if success
 * - STUB_LIB_ERR_FLASH_WRITE_UNALIGNED
 * - STUB_LIB_FAIL on ROM write error
 */
int stub_target_flash_write_buff(uint32_t addr, const void *buffer, uint32_t size, bool encrypt);

/**
 * @brief Erase entire flash chip.
 *
 * @return Result:
 * - STUB_LIB_OK if success
 * - STUB_LIB_FAIL on ROM erase error
 */
int stub_target_flash_erase_chip(void);

/**
 * @brief Erase a sector.
 *
 * @param addr Address of the sector to erase.
 * @return Result:
 * - STUB_LIB_OK if success
 * - STUB_LIB_FAIL on ROM erase error
 */
int stub_target_flash_erase_sector(uint32_t addr);

/**
 * @brief Erase a block.
 *
 * @param addr Address of the block to erase.
 * @return Result:
 * - STUB_LIB_OK if success
 * - STUB_LIB_FAIL on ROM erase error
 */
int stub_target_flash_erase_block(uint32_t addr);

/**
 * @brief Erase an area.
 *
 * @param addr Address of the area to erase.
 * @param size Size of the area to erase.
 * @return Result:
 * - STUB_LIB_OK if success
 * - STUB_LIB_FAIL on ROM erase error
 */
int stub_target_flash_erase_area(uint32_t addr, uint32_t size);

/**
 * @brief Attach SPI Flash to the hardware.
 *
 * @param ishspi SPI Flash configuration
 * @param legacy Legacy mode - disable quad mode
 */
void stub_target_flash_attach(uint32_t ishspi, bool legacy);
