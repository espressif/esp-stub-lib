/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Pin number that selects the target's default pin. */
#define STUB_LIB_NAND_PIN_DEFAULT  (-1)

// NAND registers, for stub_lib_nand_read_register()
#define NAND_REG_PROTECT           0xA0
#define NAND_REG_CONFIG            0xB0
#define NAND_REG_STATUS            0xC0

// Error codes returned by NAND functions
#define NAND_ERR_NOT_INITIALIZED   (-1)
#define NAND_ERR_ERASE_FAILED      (-2)
#define NAND_ERR_PROGRAM_FAILED    (-3)
#define NAND_ERR_ECC_UNCORRECTABLE (-4)
#define NAND_ERR_TIMEOUT           (-5)
#define NAND_ERR_PIN_INVALID       (-10)
#define NAND_ERR_SPI_FAIL          (-11)
#define NAND_ERR_GEOMETRY_UNKNOWN  (-12)
#define NAND_ERR_PROTECTION        (-50)
#define NAND_ERR_RESET_FAILED      (-100)

/**
 * @brief GPIO numbers of the SPI NAND bus
 *
 * STUB_LIB_NAND_PIN_DEFAULT selects the target's default for that pin.
 */
typedef struct {
    int8_t clk;
    int8_t q;
    int8_t d;
    int8_t cs;
    int8_t hd;
    int8_t wp; /**< Active-low write protect, driven high */
} stub_lib_nand_pins_t;

/**
 * @brief NAND geometry
 *
 * A zero field is read from the chip's parameter page.
 */
typedef struct {
    uint32_t page_size;       /**< Main area bytes per page: 2048 or 4096 */
    uint32_t pages_per_block; /**< Pages in one erase block */
    uint32_t block_count;     /**< Erase blocks on the chip */
} stub_lib_nand_geometry_t;

/**
 * @brief Initialize the SPI bus and the NAND chip
 *
 * Resets the chip, clears block protection and enables on-chip ECC.
 *
 * @param pins     Bus pins.
 * @param geometry In: geometry overrides, zero for fields to read from the chip.
 *                 Out: the geometry in use.
 * @return 0 on success,
 *         NAND_ERR_GEOMETRY_UNKNOWN when a zero field has no parameter page value,
 *         STUB_LIB_ERR_INVALID_ARG when the geometry is not supported,
 *         another negative code on bus or chip errors.
 */
int stub_lib_nand_attach(const stub_lib_nand_pins_t *pins, stub_lib_nand_geometry_t *geometry);

/**
 * @brief Read the JEDEC ID (0x9F command)
 * @param manufacturer_id Output: manufacturer ID, may be NULL
 * @param device_id Output: 2-byte device ID, may be NULL
 * @return 0 on success, negative on error
 */
int stub_lib_nand_read_id(uint8_t *manufacturer_id, uint16_t *device_id);

/**
 * @brief Read a NAND feature register
 * @param reg NAND_REG_STATUS, NAND_REG_PROTECT or NAND_REG_CONFIG
 * @param val Output byte
 * @return 0 on success, negative on error
 */
int stub_lib_nand_read_register(uint8_t reg, uint8_t *val);

/**
 * @brief Read the first 4 spare-area bytes of a page
 * @param page_number Page to read
 * @param spare_data Output buffer, at least 4 bytes and 4-byte aligned
 * @return 0 on success, negative on error
 */
int stub_lib_nand_read_bbm(uint32_t page_number, uint8_t *spare_data);

/**
 * @brief Write the bad-block marker in the spare area of a page
 * @param page_number Page to write
 * @param is_bad true to mark the block bad, false to mark it good
 * @return 0 on success, negative on error
 */
int stub_lib_nand_write_bbm(uint32_t page_number, bool is_bad);

/**
 * @brief Read the main area of one page
 * @param page_number Page to read
 * @param buf Output buffer
 * @param buf_size Bytes to read, clamped to the page size
 * @return 0 on success, NAND_ERR_ECC_UNCORRECTABLE on an ECC failure, negative on other errors
 */
int stub_lib_nand_read_page(uint32_t page_number, uint8_t *buf, uint32_t buf_size);

/**
 * @brief Program the main area of one erased page
 * @param page_number Page to write
 * @param buf Data to write
 * @param buf_size Bytes to write, clamped to the page size. 0 returns without programming.
 * @return 0 on success, NAND_ERR_PROGRAM_FAILED when the chip reports a failure, negative on other errors
 */
int stub_lib_nand_write_page(uint32_t page_number, const uint8_t *buf, uint32_t buf_size);

/**
 * @brief Erase one block
 * @param page_number First page of the block
 * @return 0 on success, NAND_ERR_ERASE_FAILED when the chip reports a failure, negative on other errors
 */
int stub_lib_nand_erase_block(uint32_t page_number);

/** @brief Page size in bytes, 0 before a successful attach */
uint32_t stub_lib_nand_get_page_size(void);

/** @brief Pages per erase block, 0 before a successful attach */
uint32_t stub_lib_nand_get_pages_per_block(void);

/** @brief Erase block size in bytes, 0 before a successful attach */
uint32_t stub_lib_nand_get_block_size(void);

/** @brief Number of erase blocks, 0 before a successful attach */
uint32_t stub_lib_nand_get_block_count(void);

#ifdef __cplusplus
}
#endif
