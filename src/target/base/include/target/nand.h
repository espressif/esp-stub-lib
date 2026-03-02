/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum time (in microseconds) to wait for any NAND operation to complete.
 * Worst-case per W25N01GV datasheet: block erase tBERS max = 10 ms. */
#define NAND_ERASE_TIMEOUT_US        1000000U

// SPI NAND Flash commands
#define NAND_CMD_SET_REGISTER        0x1F
#define NAND_CMD_READ_REGISTER       0x0F
#define NAND_CMD_WRITE_ENABLE        0x06
#define NAND_CMD_PAGE_READ           0x13
#define NAND_CMD_PROGRAM_EXECUTE     0x10
#define NAND_CMD_PROGRAM_LOAD        0x02
#define NAND_CMD_PROGRAM_LOAD_RANDOM 0x84
#define NAND_CMD_READ_FROM_CACHE     0x03
#define NAND_CMD_ERASE_BLOCK         0xD8
#define NAND_CMD_READ_ID             0x9F
#define NAND_CMD_RESET               0xFF

// NAND Flash registers
#define NAND_REG_PROTECT             0xA0
#define NAND_REG_CONFIG              0xB0
#define NAND_REG_STATUS              0xC0

// Status register bits
#define NAND_STAT_BUSY               BIT(0)
#define NAND_STAT_WRITE_ENABLED      BIT(1)
#define NAND_STAT_ERASE_FAILED       BIT(2)
#define NAND_STAT_PROGRAM_FAILED     BIT(3)

// Error codes returned by NAND functions
#define NAND_ERR_NOT_INITIALIZED     (-1)
#define NAND_ERR_ERASE_FAILED        (-2)
#define NAND_ERR_PROGRAM_FAILED      (-3)
#define NAND_ERR_TIMEOUT             (-5)
#define NAND_ERR_PIN_INVALID         (-10)
#define NAND_ERR_SPI_FAIL            (-11)
#define NAND_ERR_PROTECTION          (-50)
#define NAND_ERR_RESET_FAILED        (-100)

// Extract a 6-bit pin number from a packed hspi_arg word
#define HSPI_PIN_FIELD(arg, shift)   ((uint8_t)(((arg) >> (shift)) & 0x3FU))

// NAND flash configuration
typedef struct {
    uint32_t page_size;       // typically 2048 bytes
    uint32_t pages_per_block; // typically 64
    uint32_t block_size;      // page_size * pages_per_block
    bool initialized;
} nand_config_t;

/**
 * @brief Initialize NAND flash interface
 * @param hspi_arg SPI configuration argument.
 *
 *        On targets that support configurable GPIO routing (e.g. ESP32-S3), this
 *        argument is interpreted as a packed pin mapping with the following bit fields:
 *          - bits  0..5  : CLK GPIO number
 *          - bits  6..11 : Q (MISO) GPIO number
 *          - bits 12..17 : D (MOSI) GPIO number
 *          - bits 18..23 : CS GPIO number
 *          - bits 24..29 : HD (hold / IO2) GPIO number
 *        Remaining bits are reserved and must be zero.
 *        Pass 0 to use the default native FSPI pins.
 *
 *        On targets that do not implement pin remapping, this argument is ignored.
 * @return 0 on success, negative on error
 */
int stub_target_nand_attach(uint32_t hspi_arg);

/**
 * @brief Read bad-block marker from the spare area of a page
 * @param page_number Page number to read bad-block marker from
 * @param spare_data Output buffer for spare data (at least 2 bytes)
 * @return 0 on success, negative on error
 */
int stub_target_nand_read_bbm(uint32_t page_number, uint8_t *spare_data);

/**
 * @brief Write bad-block marker to the spare area of a page
 * @param page_number Page number to write bad-block marker to
 * @param is_bad 1 to mark as bad block, 0 to mark as good
 * @return 0 on success, negative on error
 */
int stub_target_nand_write_bbm(uint32_t page_number, uint8_t is_bad);

/**
 * @brief Read JEDEC ID from NAND flash (0x9F command)
 * @param manufacturer_id Output: manufacturer ID (0xEF for Winbond)
 * @param device_id Output: 2-byte device ID (0xAA21 for W25N01GVZEIG)
 * @return 0 on success, negative on error
 */
int stub_target_nand_read_id(uint8_t *manufacturer_id, uint16_t *device_id);

/**
 * @brief Read main area of a single NAND page
 * @param page_number Page number to read
 * @param buf Output buffer (must be at least buf_size bytes)
 * @param buf_size Number of bytes to read (up to page_size)
 * @return 0 on success, negative on error
 */
int stub_target_nand_read_page(uint32_t page_number, uint8_t *buf, uint32_t buf_size);

/**
 * @brief Write full main area of a single NAND page (2048 bytes)
 * @param page_number Page number to write
 * @param buf Data to write (must be at least page_size bytes)
 * @return 0 on success, negative on error
 */
int stub_target_nand_write_page(uint32_t page_number, const uint8_t *buf);

/**
 * @brief Erase a block (64 pages, 128KB)
 * @param page_number First page number of the block (must be block-aligned)
 * @return 0 on success, negative on error
 */
int stub_target_nand_erase_block(uint32_t page_number);

/**
 * @brief Get the configured page size
 * @return Page size in bytes
 */
uint32_t stub_target_nand_get_page_size(void);

/**
 * @brief Read a NAND configuration register (status, protect, config)
 * @param reg  Register address (NAND_REG_STATUS, NAND_REG_PROTECT, NAND_REG_CONFIG)
 * @param val  Output byte
 * @return 0 on success, negative on error
 */
int stub_target_nand_read_register(uint8_t reg, uint8_t *val);

#ifdef __cplusplus
}
#endif
