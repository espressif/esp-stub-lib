/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct stub_lib_flash_info {
    uint32_t id;
    uint32_t size;
    uint32_t block_size;
    uint32_t sector_size;
    uint32_t page_size;
    uint32_t mode; // SPI, Octal, Dual, Quad
    uint32_t encrypted;
} stub_lib_flash_info_t;

typedef struct stub_lib_flash_config {
    uint32_t flash_id;
    uint32_t flash_size;
    uint32_t block_size;
    uint32_t sector_size;
    uint32_t page_size;
    uint32_t status_mask;
} stub_lib_flash_config_t;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief Update SPI Flash configuration.
 *
 * @param config Flash configuration
 *
 * @return Result:
 * - STUB_LIB_OK if success
 * - STUB_LIB_FAIL on ROM config error
 */
int stub_lib_flash_update_config(stub_lib_flash_config_t *config);

/**
 * @brief Attach SPI Flash to the hardware.
 *
 * @param ishspi SPI Flash configuration
 * @param legacy Legacy mode - disable quad mode
 */
void stub_lib_flash_attach(uint32_t ishspi, bool legacy);

/**
 * @brief Initialize SPI Flash before any use.
 *
 * Configure SPI, Flash ID, flash size, and the internal ROM's config
 *
 * @param state Unused
 *
 * @return Error code:
 * - STUB_LIB_OK    - success
 * - STUB_LIB_ERR_UNKNOWN_FLASH_ID   - can't get size from flash id
 */
int stub_lib_flash_init(void **state);

/**
 * @brief Restore flash state at the end of the stub.
 *
 * @param state Unused.
 */
void stub_lib_flash_deinit(const void *state);

/**
 * @brief Retrieve SPI Flash information.
 *
 * @param[out] info Pointer to receive the result.
 */
void stub_lib_flash_get_info(stub_lib_flash_info_t *info);

void stub_lib_flash_info_print(const stub_lib_flash_info_t *info);

/**
 * @brief Read data from SPI flash into a buffer.
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
int stub_lib_flash_read_buff(uint32_t addr, void *buffer, uint32_t size);
int stub_lib_flash_write_buff(uint32_t addr, const void *buffer, uint32_t size, int encrypt);
int stub_lib_flash_erase_area(uint32_t addr, uint32_t size);
int stub_lib_flash_erase_sector(uint32_t addr);
int stub_lib_flash_erase_block(uint32_t addr);
int stub_lib_flash_erase_chip(void);

#ifdef __cplusplus
}
#endif // __cplusplus
