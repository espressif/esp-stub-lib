/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/* Flash geometry constants */
#define STUB_FLASH_SECTOR_SIZE          0x1000U
#define STUB_FLASH_BLOCK_SIZE           0x10000U
#define STUB_FLASH_PAGE_SIZE            0x100U
#define STUB_FLASH_STATUS_MASK          0xFFFFU

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

/**
 * @brief Wait for SPI flash to become ready or timeout
 *
 * Polls stub_target_flash_is_busy() and counts elapsed time using stub_lib_delay_us(1).
 * Use timeout_us 0 for a single non-blocking check (returns STUB_LIB_ERR_TIMEOUT if busy).
 *
 * @param timeout_us Maximum time to wait in microseconds
 * @return STUB_LIB_OK on success (flash ready), STUB_LIB_ERR_TIMEOUT on timeout
 */
int stub_lib_flash_wait_ready(uint64_t timeout_us);

/**
 * @brief Start erasing the next sector or block (async)
 *
 * This function initiates an erase operation and returns immediately without
 * waiting for completion. It determines whether to erase a 4KB sector or
 * 64KB block based on alignment and remaining size.
 *
 * @param next_erase_addr Pointer to the next address to erase (will be updated)
 * @param remaining_size Pointer to remaining bytes to erase (will be updated)
 *
 * @return Result:
 * - STUB_LIB_OK if success
 * - STUB_LIB_ERR_INVALID_ARG if invalid arguments
 * - STUB_LIB_ERR_FLASH_BUSY if flash is busy
 */
int stub_lib_flash_start_next_erase(uint32_t *next_erase_addr, uint32_t *remaining_size);

#ifdef __cplusplus
}
#endif // __cplusplus
