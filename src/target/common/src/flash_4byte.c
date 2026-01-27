/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
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

static void page_program_4b(int spi_num, uint32_t flash_addr, const uint8_t *data, uint32_t size)
{
    uint32_t current_addr = flash_addr;
    const uint8_t *current_data = data;
    uint32_t remaining = size;
    uint32_t chunk_size;

    while (stub_target_flash_is_busy());
    while (remaining > 0) {
        stub_target_flash_write_enable();
        chunk_size = (remaining >= 32) ? 32 : remaining;  /* write_sub_len = 32 */
        stub_target_opiflash_exec_cmd(spi_num, SPI_FLASH_FASTRD_MODE,
                                      CMD_PROGRAM_PAGE_4B, 8,
                                      current_addr, 32,
                                      0,
                                      (uint8_t *)current_data, (int)(8 * chunk_size),
                                      NULL, 0,
                                      ESP_ROM_OPIFLASH_SEL_CS0,
                                      true);
        while (stub_target_flash_is_busy());
        current_data += chunk_size;
        current_addr += chunk_size;
        remaining -= chunk_size;
    }
}

static int stub_target_flash_4byte_write_unencrypted(int spi_num, uint32_t flash_addr, const uint8_t *data,
                                                     uint32_t size)
{
    uint32_t page_size = 256;
    uint32_t bytes_written, full_pages;
    uint32_t i;

    bytes_written = page_size - (flash_addr % page_size);
    if (size < bytes_written) {
        page_program_4b(spi_num, flash_addr, data, size);
    } else {
        page_program_4b(spi_num, flash_addr, data, bytes_written);
        /* Whole page program */
        full_pages = (size - bytes_written) / page_size;
        for (i = 0; i < full_pages; i++) {
            page_program_4b(spi_num, flash_addr + bytes_written, data + bytes_written, page_size);
            bytes_written += page_size;
        }
        /* Remaining parts to program */
        page_program_4b(spi_num, flash_addr + bytes_written, data + bytes_written, size - bytes_written);
    }
    while (stub_target_flash_is_busy());
    return 0;
}

static int stub_target_flash_4byte_write_encrypted(int spi_num, uint32_t flash_addr, const uint8_t *data, uint32_t size)
{
    if (flash_addr % 16 != 0 || size % 16 != 0) {
        // Minimum 128-bit size & alignment
        STUB_LOGE("Unaligned write: 0x%x, %u\n", flash_addr, size);
        return STUB_LIB_ERR_FLASH_WRITE_UNALIGNED;
    }

    stub_target_flash_write_encrypted_enable();
    stub_target_aes_xts_init();

    if (stub_target_flash_unlock() != STUB_LIB_OK) {
        stub_target_flash_write_encrypted_disable();
        return STUB_LIB_ERR_FLASH_WRITE;
    }

    while (size > 0) {
        uint8_t block_size;
        const uint32_t timeout_us = 100000;
        uint16_t page_size = 256;
        if (flash_addr % 64 == 0 && size >= 64 && (page_size - flash_addr % page_size >= 64)) {
            block_size = 64;
        } else if (flash_addr % 32 == 0 && size >= 32 && (page_size - flash_addr % page_size >= 32)) {
            block_size = 32;
        } else {
            block_size = 16;
        }

        stub_target_aes_xts_encrypt_trigger(flash_addr, (void *)data, block_size);
        stub_target_flash_write_enable();
        if (stub_target_aes_xts_wait_data_ready(timeout_us) != STUB_LIB_OK) {
            stub_target_flash_write_encrypted_disable();
            return STUB_LIB_ERR_FLASH_WRITE;
        }
        while (stub_target_flash_is_busy());

        stub_target_opiflash_exec_cmd(spi_num, SPI_FLASH_FASTRD_MODE,
                                      CMD_PROGRAM_PAGE_4B, 8,
                                      flash_addr, 32,
                                      0,
                                      NULL, (int)(8 * block_size - 1),
                                      NULL, 0,
                                      ESP_ROM_OPIFLASH_SEL_CS0,
                                      true);
        stub_target_aes_xts_destroy();

        size -= block_size;
        data += block_size;
        flash_addr += block_size;
    }
    while (stub_target_flash_is_busy());
    stub_target_flash_write_encrypted_disable();
    return STUB_LIB_OK;
}

int stub_target_flash_4byte_write(int spi_num, uint32_t flash_addr, const uint8_t *data, uint32_t size, bool encrypt)
{
    if (encrypt) {
        return stub_target_flash_4byte_write_encrypted(spi_num, flash_addr, data, size);
    }
    return stub_target_flash_4byte_write_unencrypted(spi_num, flash_addr, data, size);
}

int stub_target_flash_4byte_read(int spi_num, uint32_t flash_addr, uint8_t *buffer, uint32_t size)
{
    uint8_t cmd_len = 8;
    while (stub_target_flash_is_busy());
    while (size > 0) {
        uint32_t chunk_size = (size > 16) ? 16 : size;  /* read_sub_len = 16 */
        stub_target_opiflash_exec_cmd(spi_num, SPI_FLASH_FASTRD_MODE,
                                      CMD_FSTRD4B, cmd_len,
                                      flash_addr, 32,
                                      8,
                                      NULL, 0,
                                      buffer, (int)(8 * chunk_size),
                                      ESP_ROM_OPIFLASH_SEL_CS0,
                                      false);

        size -= chunk_size;
        buffer += chunk_size;
        flash_addr += chunk_size;
    }
    return 0;
}

void stub_target_flash_4byte_erase_sector_start(int spi_num, uint32_t flash_addr)
{
    stub_target_flash_write_enable();
    while (stub_target_flash_is_busy());

    stub_target_opiflash_exec_cmd(spi_num, SPI_FLASH_SLOWRD_MODE,
                                  CMD_SECTOR_ERASE_4B, 8,
                                  flash_addr, 32,
                                  0,
                                  NULL, 0,
                                  NULL, 0,
                                  ESP_ROM_OPIFLASH_SEL_CS0,
                                  true);
}

void stub_target_flash_4byte_erase_block_start(int spi_num, uint32_t flash_addr)
{
    stub_target_flash_write_enable();
    while (stub_target_flash_is_busy());

    stub_target_opiflash_exec_cmd(spi_num, SPI_FLASH_SLOWRD_MODE,
                                  CMD_LARGE_BLOCK_ERASE_4B, 8,
                                  flash_addr, 32,
                                  0,
                                  NULL, 0,
                                  NULL, 0,
                                  ESP_ROM_OPIFLASH_SEL_CS0,
                                  true);
}

int stub_target_flash_4byte_erase_sector(int spi_num, uint32_t flash_addr)
{
    stub_target_flash_write_enable();
    while (stub_target_flash_is_busy());

    stub_target_opiflash_exec_cmd(spi_num, SPI_FLASH_SLOWRD_MODE,
                                  CMD_SECTOR_ERASE_4B, 8,
                                  flash_addr, 32,
                                  0,
                                  NULL, 0,
                                  NULL, 0,
                                  ESP_ROM_OPIFLASH_SEL_CS0,
                                  true);

    while (stub_target_flash_is_busy());
    return 0;
}

int stub_target_flash_4byte_erase_block(int spi_num, uint32_t flash_addr)
{
    stub_target_flash_write_enable();
    while (stub_target_flash_is_busy());

    stub_target_opiflash_exec_cmd(spi_num, SPI_FLASH_SLOWRD_MODE,
                                  CMD_LARGE_BLOCK_ERASE_4B, 8,
                                  flash_addr, 32,
                                  0,
                                  NULL, 0,
                                  NULL, 0,
                                  ESP_ROM_OPIFLASH_SEL_CS0,
                                  true);

    while (stub_target_flash_is_busy());
    return 0;
}
