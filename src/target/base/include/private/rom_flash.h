/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief SPI flash read mode, mirrors the ROM's SpiFlashRdMode
 */
typedef enum {
    SPI_FLASH_QIO_MODE = 0,
    SPI_FLASH_QOUT_MODE,
    SPI_FLASH_DIO_MODE,
    SPI_FLASH_DOUT_MODE,
    SPI_FLASH_FASTRD_MODE,
    SPI_FLASH_SLOWRD_MODE,
    SPI_FLASH_OPI_STR_MODE,
    SPI_FLASH_OPI_DTR_MODE,
    SPI_FLASH_OOUT_MODE,
    SPI_FLASH_OIO_STR_MODE,
    SPI_FLASH_OIO_DTR_MODE,
    SPI_FLASH_QPI_MODE,
    SPI_FLASH_OPI_HEX_DTR_MODE,
} spi_flash_mode_t;

typedef struct {
    uint32_t flash_id;
    uint32_t chip_size; // chip size in bytes
    uint32_t block_size;
    uint32_t sector_size;
    uint32_t page_size;
    uint32_t status_mask;
} esp_rom_spiflash_chip_t;

enum { ESP_ROM_SPIFLASH_RESULT_OK, ESP_ROM_SPIFLASH_RESULT_ERR, ESP_ROM_SPIFLASH_RESULT_TIMEOUT };

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
extern int esp_rom_spiflash_config_param(uint32_t flash_id,
                                         uint32_t chip_size,
                                         uint32_t block_size,
                                         uint32_t sector_size,
                                         uint32_t page_size,
                                         uint32_t status_mask);

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
int esp_rom_spiflash_read(uint32_t src_addr, uint32_t *dest, int32_t len);

/**
 * @brief Unlock SPI Flash
 *
 * @return Result
 * - ESP_ROM_SPIFLASH_RESULT_OK
 * - ESP_ROM_SPIFLASH_RESULT_ERR
 */
int esp_rom_spiflash_unlock(void);

/**
 * @brief Erase an area of SPI flash using ROM implementation.
 *
 * @return Result
 * - ESP_ROM_SPIFLASH_RESULT_OK
 * - ESP_ROM_SPIFLASH_RESULT_ERR
 * - ESP_ROM_SPIFLASH_RESULT_TIMEOUT
 */
int esp_rom_spiflash_erase_area(uint32_t start_addr, uint32_t area_len);

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

/**
 * @brief Dispatch table of the ROM SPI user-command primitives
 *
 * The functions themselves live in revision specific ROM sections and move
 * between revisions, but the table is populated by the ROM, so calling through
 * it works on any revision. Only the table pointer needs a fixed address.
 *
 * Only exec_flash_cmd is typed; the preceding members exist to place it at the
 * right offset.
 */
typedef struct {
    void *set_mode;
    void *reset_mode;
    void *cmd_start;
    void *usr_cmd_config;
    void *conf_flash_cmd;
    void (*exec_flash_cmd)(int spi_num,
                           spi_flash_mode_t mode,
                           uint32_t cmd,
                           int cmd_bit_len,
                           uint32_t addr,
                           int addr_bit_len,
                           int dummy_bits,
                           const uint8_t *mosi_data,
                           int mosi_bit_len,
                           uint8_t *miso_data,
                           int miso_bit_len,
                           uint32_t cs_mask,
                           bool is_write_erase_operation);
    void *reg_backup;
    void *reg_recover;
    uint32_t *regs;
} esp_rom_spi_usr_cmd_legacy_funcs_t;

extern const esp_rom_spi_usr_cmd_legacy_funcs_t *rom_spi_usr_cmd_legacy_funcs;
