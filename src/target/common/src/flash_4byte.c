/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <target/flash.h>
#include <target/aes_xts.h>
#include <private/rom_flash.h>
#include <private/flash_commands.h>
#include <target/flash_4byte.h>
#include <esp-stub-lib/log.h>
#include <esp-stub-lib/err.h>
#include <esp-stub-lib/flash.h>

// Timeout values for flash operations, inspired by esptool
#define DEFAULT_TIMEOUT_US 10000U
#define ERASE_PER_KB_TIMEOUT_US 30000U
#define WRITE_PER_KB_TIMEOUT_US 30000U

// AES-XTS encryption block sizes for encrypted flash write operations
// The hardware AES-XTS engine can process multiple 128-bit blocks simultaneously when
// the flash address is properly aligned, providing better performance with larger blocks.
#define AES_XTS_MIN_BLOCK_SIZE    16
#define AES_XTS_DOUBLE_BLOCK_SIZE 32
#define AES_XTS_QUAD_BLOCK_SIZE   64

// Flash write chunk size for unencrypted operations
// Taken from ROM SPI_BUFF_BYTE_WRITE_NUM
#define FLASH_WRITE_CHUNK_SIZE    32

// Flash read chunk size for 4-byte address mode operations
// Taken from ROM SPI_BUFF_BYTE_READ_NUM
#define FLASH_READ_CHUNK_SIZE     16

static void exec_write_cmd_4b(int spi_num, uint32_t flash_addr, const uint8_t *data, int bit_len)
{
    stub_target_opiflash_exec_cmd(&(opiflash_cmd_params_t) {
        .spi_num = spi_num,
        .mode = SPI_FLASH_FASTRD_MODE,
        .cmd = CMD_PROGRAM_PAGE_4B,
        .cmd_bit_len = 8,
        .addr = flash_addr,
        .addr_bit_len = 32,
        .dummy_bits = 0,
        .mosi_data = data,
        .mosi_bit_len = bit_len,
        .miso_data = NULL,
        .miso_bit_len = 0,
        .cs_mask = ESP_ROM_OPIFLASH_SEL_CS0,
        .is_write_erase_operation = true
    });
}

static void exec_read_cmd_4b(int spi_num, uint32_t flash_addr, uint8_t *buffer, int bit_len)
{
    stub_target_opiflash_exec_cmd(&(opiflash_cmd_params_t) {
        .spi_num = spi_num,
        .mode = SPI_FLASH_FASTRD_MODE,
        .cmd = CMD_FSTRD4B,
        .cmd_bit_len = 8,
        .addr = flash_addr,
        .addr_bit_len = 32,
        .dummy_bits = 8,
        .mosi_data = NULL,
        .mosi_bit_len = 0,
        .miso_data = buffer,
        .miso_bit_len = bit_len,
        .cs_mask = ESP_ROM_OPIFLASH_SEL_CS0,
        .is_write_erase_operation = false
    });
}

static void exec_erase_cmd_4b(int spi_num, uint32_t flash_addr, uint8_t erase_cmd)
{
    stub_target_opiflash_exec_cmd(&(opiflash_cmd_params_t) {
        .spi_num = spi_num,
        .mode = SPI_FLASH_SLOWRD_MODE,
        .cmd = erase_cmd,
        .cmd_bit_len = 8,
        .addr = flash_addr,
        .addr_bit_len = 32,
        .dummy_bits = 0,
        .mosi_data = NULL,
        .mosi_bit_len = 0,
        .miso_data = NULL,
        .miso_bit_len = 0,
        .cs_mask = ESP_ROM_OPIFLASH_SEL_CS0,
        .is_write_erase_operation = true
    });
}

/**
 * Select chunk size for unencrypted write
 */
static inline uint32_t get_write_chunk_size_unencrypted(uint32_t remaining, uint32_t page_space)
{
    uint32_t chunk = (remaining >= FLASH_WRITE_CHUNK_SIZE) ? FLASH_WRITE_CHUNK_SIZE : remaining;
    return (chunk <= page_space) ? chunk : page_space;
}

/**
 * Select block size for encrypted write based on address alignment
 * Prefers larger blocks (64 → 32 → 16 bytes) for better AES-XTS performance
 */
static inline uint8_t get_write_block_size_encrypted(uint32_t flash_addr, uint32_t remaining, uint32_t page_space)
{
    if (IS_ALIGNED(flash_addr, AES_XTS_QUAD_BLOCK_SIZE) && remaining >= AES_XTS_QUAD_BLOCK_SIZE
            && page_space >= AES_XTS_QUAD_BLOCK_SIZE) {
        return AES_XTS_QUAD_BLOCK_SIZE;
    } else if (IS_ALIGNED(flash_addr, AES_XTS_DOUBLE_BLOCK_SIZE) && remaining >= AES_XTS_DOUBLE_BLOCK_SIZE
               && page_space >= AES_XTS_DOUBLE_BLOCK_SIZE) {
        return AES_XTS_DOUBLE_BLOCK_SIZE;
    }
    return AES_XTS_MIN_BLOCK_SIZE;
}

static int stub_target_flash_4byte_write_unencrypted(int spi_num, uint32_t flash_addr, const uint8_t *data,
                                                     uint32_t size, uint64_t timeout_us)
{
    if (stub_lib_flash_wait_ready(timeout_us) != STUB_LIB_OK) {
        return STUB_LIB_ERR_TIMEOUT;
    }

    while (size > 0) {
        uint32_t page_space = STUB_FLASH_PAGE_SIZE - (flash_addr % STUB_FLASH_PAGE_SIZE);
        uint32_t chunk_size = get_write_chunk_size_unencrypted(size, page_space);

        stub_target_flash_write_enable();
        exec_write_cmd_4b(spi_num, flash_addr, data, (int)(8 * chunk_size));

        if (stub_lib_flash_wait_ready(timeout_us) != STUB_LIB_OK) {
            return STUB_LIB_ERR_TIMEOUT;
        }

        data += chunk_size;
        flash_addr += chunk_size;
        size -= chunk_size;
    }
    return STUB_LIB_OK;
}

static int stub_target_flash_4byte_write_encrypted(int spi_num, uint32_t flash_addr, const uint8_t *data, uint32_t size,
                                                   uint64_t timeout_us)
{
    int ret = STUB_LIB_ERR_TIMEOUT;

    if (!IS_ALIGNED(flash_addr, AES_XTS_MIN_BLOCK_SIZE) || !IS_ALIGNED(size, AES_XTS_MIN_BLOCK_SIZE)) {
        // Minimum 128-bit size & alignment for AES-XTS encryption
        STUB_LOGE("Unaligned write: 0x%x, %u\n", flash_addr, size);
        return STUB_LIB_ERR_FLASH_WRITE_UNALIGNED;
    }

    stub_target_flash_write_encrypted_enable();
    stub_target_aes_xts_init();

    if (stub_target_flash_unlock() != STUB_LIB_OK) {
        ret = STUB_LIB_FAIL;
        goto cleanup;
    }

    if (stub_lib_flash_wait_ready(timeout_us) != STUB_LIB_OK) {
        goto cleanup;
    }

    while (size > 0) {
        uint32_t page_space = STUB_FLASH_PAGE_SIZE - (flash_addr % STUB_FLASH_PAGE_SIZE);
        uint8_t block_size = get_write_block_size_encrypted(flash_addr, size, page_space);

        stub_target_aes_xts_encrypt_trigger(flash_addr, data, block_size);
        stub_target_flash_write_enable();

        if (stub_target_aes_xts_wait_data_ready(timeout_us) != STUB_LIB_OK) {
            goto cleanup;
        }

        // Note: mosi_bit_len uses -1, possibly related to hardware bit counter (0-based), not clear why, should be investigated.
        exec_write_cmd_4b(spi_num, flash_addr, NULL, (int)(8 * block_size - 1));
        stub_target_aes_xts_clear();

        if (stub_lib_flash_wait_ready(timeout_us) != STUB_LIB_OK) {
            goto cleanup;
        }

        data += block_size;
        flash_addr += block_size;
        size -= block_size;
    }

    ret = STUB_LIB_OK;

cleanup:
    stub_target_flash_write_encrypted_disable();
    return ret;
}

int stub_target_flash_4byte_write(int spi_num, uint32_t flash_addr, const uint8_t *data, uint32_t size, bool encrypt)
{
    uint64_t timeout_us = size / 1024 * WRITE_PER_KB_TIMEOUT_US;
    if (encrypt) {
        return stub_target_flash_4byte_write_encrypted(spi_num, flash_addr, data, size, timeout_us);
    }
    return stub_target_flash_4byte_write_unencrypted(spi_num, flash_addr, data, size, timeout_us);
}

int stub_target_flash_4byte_read(int spi_num, uint32_t flash_addr, uint8_t *buffer, uint32_t size)
{
    if (stub_lib_flash_wait_ready(DEFAULT_TIMEOUT_US) != STUB_LIB_OK) {
        return STUB_LIB_ERR_TIMEOUT;
    }
    while (size > 0) {
        uint32_t chunk_size = (size > FLASH_READ_CHUNK_SIZE) ? FLASH_READ_CHUNK_SIZE : size;
        exec_read_cmd_4b(spi_num, flash_addr, buffer, (int)(8 * chunk_size));

        size -= chunk_size;
        buffer += chunk_size;
        flash_addr += chunk_size;
    }
    return 0;
}

static int erase_start_4b(int spi_num, uint32_t flash_addr, uint8_t erase_cmd)
{
    if (stub_lib_flash_wait_ready(DEFAULT_TIMEOUT_US) != STUB_LIB_OK) {
        return STUB_LIB_ERR_TIMEOUT;
    }

    stub_target_flash_write_enable();
    exec_erase_cmd_4b(spi_num, flash_addr, erase_cmd);
    return STUB_LIB_OK;
}

int stub_target_flash_4byte_erase_sector_start(int spi_num, uint32_t flash_addr)
{
    return erase_start_4b(spi_num, flash_addr, CMD_SECTOR_ERASE_4B);
}

int stub_target_flash_4byte_erase_block_start(int spi_num, uint32_t flash_addr)
{
    return erase_start_4b(spi_num, flash_addr, CMD_LARGE_BLOCK_ERASE_4B);
}

int stub_target_flash_4byte_erase_sector(int spi_num, uint32_t flash_addr)
{
    int ret = erase_start_4b(spi_num, flash_addr, CMD_SECTOR_ERASE_4B);
    if (ret != STUB_LIB_OK) {
        return ret;
    }

    uint64_t timeout_us = STUB_FLASH_SECTOR_SIZE / 1024 * ERASE_PER_KB_TIMEOUT_US;
    if (stub_lib_flash_wait_ready(timeout_us) != STUB_LIB_OK) {
        return STUB_LIB_ERR_TIMEOUT;
    }
    return STUB_LIB_OK;
}

int stub_target_flash_4byte_erase_block(int spi_num, uint32_t flash_addr)
{
    int ret = erase_start_4b(spi_num, flash_addr, CMD_LARGE_BLOCK_ERASE_4B);
    if (ret != STUB_LIB_OK) {
        return ret;
    }

    uint64_t timeout_us = STUB_FLASH_BLOCK_SIZE / 1024 * ERASE_PER_KB_TIMEOUT_US;
    if (stub_lib_flash_wait_ready(timeout_us) != STUB_LIB_OK) {
        return STUB_LIB_ERR_TIMEOUT;
    }
    return STUB_LIB_OK;
}
