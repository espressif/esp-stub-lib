/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Write data using 4-byte addressing
 *
 * @param spi_num SPI peripheral number (typically 1)
 * @param flash_addr Flash address to write to
 * @param data Source data buffer
 * @param size Length of data to write
 * @param encrypt Whether to use encrypted write
 * @return Result:
 * - STUB_LIB_OK
 * - STUB_LIB_ERR_FLASH_WRITE_UNALIGNED
 * - STUB_LIB_ERR_TIMEOUT
 * - STUB_LIB_FAIL

 */
int stub_target_flash_4byte_write(int spi_num, uint32_t flash_addr, const uint8_t *data, uint32_t size, bool encrypt);

/**
 * @brief Read data using 4-byte addressing
 *
 * @param spi_num SPI peripheral number (typically 1)
 * @param flash_addr Flash address to read from
 * @param buffer Buffer to read data into
 * @param size Length of data to read
 * @param dout Read in dual output mode instead of single line fast read
 * @return Result:
 * - STUB_LIB_OK
 * - STUB_LIB_ERR_TIMEOUT
 */
int stub_target_flash_4byte_read(int spi_num, uint32_t flash_addr, uint8_t *buffer, uint32_t size, bool dout);

/**
 * @brief Read data using 4-byte addressing from flash that is ready
 *
 * The default sends one ROM user command for each 64 bytes. A target can
 * replace it with one that configures the SPI controller once.
 *
 * @param spi_num SPI peripheral number (typically 1)
 * @param flash_addr Flash address to read from
 * @param buffer Buffer to read data into
 * @param size Length of data to read
 * @param dout Read in dual output mode instead of single line fast read
 */
void stub_target_flash_4byte_read_data(int spi_num, uint32_t flash_addr, uint8_t *buffer, uint32_t size, bool dout);

/**
 * @brief Program unencrypted data using 4-byte addressing
 *
 * Waits until the flash is ready before and after each Page Program command.
 * The default sends each command through the ROM. A target can replace it
 * with one that configures the SPI controller once.
 *
 * @param spi_num SPI peripheral number (typically 1)
 * @param flash_addr Flash address to write to
 * @param data Source data buffer
 * @param size Length of data to write
 * @param timeout_us Timeout of each wait for the flash
 * @return Result:
 * - STUB_LIB_OK
 * - STUB_LIB_ERR_TIMEOUT
 */
int stub_target_flash_4byte_program(int spi_num,
                                    uint32_t flash_addr,
                                    const uint8_t *data,
                                    uint32_t size,
                                    uint64_t timeout_us);

/**
 * @brief Start sector erase using 4-byte addressing (non-blocking)
 *
 * Triggers the sector erase command and returns immediately.
 * Caller should check if flash is busy before proceeding.
 *
 * @param spi_num SPI peripheral number (typically 1)
 * @param flash_addr Sector address to erase
 * @return Result:
 * - STUB_LIB_OK
 * - STUB_LIB_ERR_TIMEOUT
 */
int stub_target_flash_4byte_erase_sector_start(int spi_num, uint32_t flash_addr);

/**
 * @brief Start block erase using 4-byte addressing (non-blocking)
 *
 * Triggers the 64KB block erase command and returns immediately.
 * Caller should check if flash is busy before proceeding.
 *
 * @param spi_num SPI peripheral number (typically 1)
 * @param flash_addr Block address to erase
 * @return Result:
 * - STUB_LIB_OK
 * - STUB_LIB_ERR_TIMEOUT
 */
int stub_target_flash_4byte_erase_block_start(int spi_num, uint32_t flash_addr);

/**
 * @brief Erase sector using 4-byte addressing (blocking)
 *
 * Performs a sector erase and waits for completion.
 *
 * @param spi_num SPI peripheral number (typically 1)
 * @param flash_addr Sector address to erase
 * @return Result:
 * - STUB_LIB_OK
 * - STUB_LIB_ERR_TIMEOUT
 */
int stub_target_flash_4byte_erase_sector(int spi_num, uint32_t flash_addr);

/**
 * @brief Erase 64KB block using 4-byte addressing (blocking)
 *
 * Performs a block erase and waits for completion.
 *
 * @param spi_num SPI peripheral number (typically 1)
 * @param flash_addr Block address to erase
 * @return Result:
 * - STUB_LIB_OK
 * - STUB_LIB_ERR_TIMEOUT
 */
int stub_target_flash_4byte_erase_block(int spi_num, uint32_t flash_addr);
