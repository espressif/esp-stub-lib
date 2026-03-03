/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <private/rom_flash.h>

/**
 * @brief Flash geometry constants
 */
#define STUB_FLASH_SECTOR_SIZE 0x1000U
#define STUB_FLASH_BLOCK_SIZE  0x10000U
#define STUB_FLASH_PAGE_SIZE   0x100U
#define STUB_FLASH_STATUS_MASK 0xFFFFU

/**
 * @brief SPI flash read mode for OPI operations
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
} spi_flash_mode_t;

#define FLASH_SPI_NUM 1

/**
 * @brief This function saves the state of the SPI flash
 *
 * @param state State pointer.
 *
 * @return true if attach is required, false if not
 */
bool stub_target_flash_state_save(void **state);

/**
 * @brief Reset default SPI IOMUX pins to GPIO mode
 *
 * This function configures the default SPI flash IOMUX pins as GPIOs.
 * This is needed when switching from default IOMUX routing to GPIO matrix routing
 * to avoid bus conflicts. Should be called before attaching with custom SPI pins.
 */
void stub_target_reset_default_spi_pins(void);

/**
 * @brief Initialize SPI Flash hardware.
 *
 * Configure SPI pins, registers, mode, etc.
 *
 * @param state State pointer.
 */
void stub_target_flash_init(void **state);

/**
 * @brief Restore the state of the SPI flash
 *
 * @param state State pointer.
 */
void stub_target_flash_state_restore(const void *state);

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
esp_rom_spiflash_chip_t *stub_target_flash_get_config(void);

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
int stub_target_flash_update_config(uint32_t flash_id,
                                    uint32_t flash_size,
                                    uint32_t block_size,
                                    uint32_t sector_size,
                                    uint32_t page_size,
                                    uint32_t status_mask);

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
 * @brief Get maximum supported flash size for the target chip
 *
 * Returns the maximum flash size that the target chip can support,
 * taking into account addressing limitations and hardware capabilities.
 * This is used as a fallback when flash ID is unknown.
 *
 * @return Maximum supported flash size in bytes
 */
uint32_t stub_target_get_max_supported_flash_size(void);

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
 * - STUB_LIB_ERR_FLASH_READ
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
 * - STUB_LIB_ERR_FLASH_WRITE
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
 * @brief Attach SPI Flash to the hardware.
 *
 * @param ishspi SPI Flash configuration
 * @param legacy Legacy mode - disable quad mode
 */
void stub_target_flash_attach(uint32_t ishspi, bool legacy);

/**
 * @brief Check if the SPI flash is busy (non-blocking)
 *
 * Target-specific implementations should check the flash busy status without blocking.
 *
 * @return true if flash is busy, false if ready
 */
bool stub_target_flash_is_busy(void);

void stub_target_spi_wait_ready(void);

/**
 * @brief Start erasing a 4KB sector without blocking
 *
 * Target implementations should trigger the sector erase command and return
 * without waiting for completion.
 *
 * @param addr Sector address (guaranteed to be 4KB aligned by caller)
 */
void stub_target_flash_erase_sector_start(uint32_t addr);

/**
 * @brief Start erasing a 64KB block without blocking
 *
 * Target implementations should trigger the block erase command and return
 * without waiting for completion.
 *
 * @param addr Block address (guaranteed to be 64KB aligned by caller)
 */
void stub_target_flash_erase_block_start(uint32_t addr);

/**
 * @brief Enable writes to flash
 *
 * Target-specific implementation to enable flash write operations.
 * Used by 4-byte flash operations during page programming.
 * Each target that supports large flash must implement this.
 */
void stub_target_flash_write_enable(void);

/**
 * @brief Parameters for OPI flash command execution
 *
 * This structure encapsulates all parameters needed to execute an arbitrary
 * flash command with configurable parameters.
 */
typedef struct opiflash_cmd_params {
    int spi_num;                   /**< SPI peripheral number (typically 1 for flash) */
    spi_flash_mode_t mode;         /**< Read mode (SLOWRD, FASTRD, etc.) */
    uint32_t cmd;                  /**< Command opcode */
    int cmd_bit_len;               /**< Command length in bits */
    uint32_t addr;                 /**< Address value */
    int addr_bit_len;              /**< Address length in bits (32 for 4-byte addressing) */
    int dummy_bits;                /**< Number of dummy clock cycles */
    const uint8_t *mosi_data;      /**< Data to send (write operations) */
    int mosi_bit_len;              /**< Length of data to send in bits */
    uint8_t *miso_data;            /**< Buffer to receive data (read operations) */
    int miso_bit_len;              /**< Length of data to receive in bits */
    uint32_t cs_mask;              /**< Chip select mask (ESP_ROM_OPIFLASH_SEL_CS0 or CS1) */
    bool is_write_erase_operation; /**< True for write/erase, false for read */
} opiflash_cmd_params_t;

/**
 * @brief Execute a generic SPI/OPI flash command
 *
 * This function is available on chips supporting >16 MB flash.
 * It allows execution of arbitrary flash commands with configurable parameters.
 *
 * @param params Pointer to structure containing all command parameters
 */
void stub_target_opiflash_exec_cmd(const opiflash_cmd_params_t *params);

/**
 * @brief Enable encrypted writes to flash
 *
 * Target-specific implementation to enable encrypted flash write operations.
 * Used by 4-byte flash operations during page programming.
 * Each target that supports large flash must implement this.
 */
void stub_target_flash_write_encrypted_enable(void);

/**
 * @brief Disable encrypted writes to flash
 *
 * Target-specific implementation to disable encrypted flash write operations.
 * Used by 4-byte flash operations during page programming.
 * Each target that supports large flash must implement this.
 */
void stub_target_flash_write_encrypted_disable(void);

/**
 * @brief Unlock the flash
 *
 * Target-specific implementation to unlock the flash.
 * Used by 4-byte flash operations during page programming.
 * Each target that supports large flash must implement this.
 */
int stub_target_flash_unlock(void);
