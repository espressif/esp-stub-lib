/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>

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
 */
void stub_target_flash_update_config(uint32_t flash_id, uint32_t flash_size);

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
