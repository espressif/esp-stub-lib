/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <flash.h>
#include <err.h>
#include <log.h>
#include <bit_utils.h>
#include <stddef.h>
#include <target/flash.h>
#include <target/flash_4byte.h>
#include <private/rom_flash_config.h>
#include <private/rom_flash.h>
#include <rom_wrappers.h>

// For flash size > 16MB, we use 4-byte addressing, only some targets support this.
static bool large_flash_mode = false;

#define FLASH_SPI_NUM 1

int stub_lib_flash_update_config(stub_lib_flash_config_t *config)
{
    if (config == NULL) {
        return STUB_LIB_ERR_INVALID_ARG;
    }

    return stub_target_flash_update_config(config->flash_id,
                                           config->flash_size,
                                           config->block_size,
                                           config->sector_size,
                                           config->page_size,
                                           config->status_mask);
}

void stub_lib_flash_attach(uint32_t ishspi, bool legacy)
{
    stub_target_reset_default_spi_pins();
    stub_target_flash_attach(ishspi, legacy);
}

int stub_lib_flash_init(void **state)
{
    stub_target_flash_init(state);

    uint32_t flash_id = stub_target_flash_get_flash_id();
    uint32_t flash_size = stub_target_flash_id_to_flash_size(flash_id);

    int return_code = STUB_LIB_OK;

    STUB_LOG_TRACEF("Flash size: %d MB, flash_id: 0x%x\n", BYTES_TO_MIB(flash_size), flash_id);

    if (flash_size == 0) {
        /* Unknown flash ID - use target-specific maximum supported size as fallback */
        flash_size = stub_target_get_max_supported_flash_size();
        STUB_LOGI("Unknown flash ID, assuming %d MB (chip max)\n", BYTES_TO_MIB(flash_size));
        return_code = STUB_LIB_ERR_UNKNOWN_FLASH_ID;
    }

    if (flash_size > MIB(16)) {
        large_flash_mode = true;
    }

    int config_ret = stub_target_flash_update_config(flash_id,
                                                     flash_size,
                                                     STUB_FLASH_BLOCK_SIZE,
                                                     STUB_FLASH_SECTOR_SIZE,
                                                     STUB_FLASH_PAGE_SIZE,
                                                     STUB_FLASH_STATUS_MASK);
    if (config_ret != STUB_LIB_OK) {
        return config_ret;
    }

    return return_code;
}

void stub_lib_flash_deinit(const void *state)
{
    stub_target_flash_deinit(state);
}

void stub_lib_flash_get_config(stub_lib_flash_config_t *cfg)
{
    const esp_rom_spiflash_chip_t *chip = stub_target_flash_get_config();

    cfg->flash_id = chip->flash_id;
    cfg->flash_size = chip->chip_size;
    cfg->block_size = chip->block_size;
    cfg->sector_size = chip->sector_size;
    cfg->page_size = chip->page_size;
    cfg->status_mask = chip->status_mask;
}

int stub_lib_flash_read_buff(uint32_t addr, void *buffer, uint32_t size)
{
    STUB_LOG_TRACEF("addr: 0x%x, size: %u, large_flash_mode: %d\n", addr, size, large_flash_mode);

    if (!IS_ALIGNED(addr, 4) || !IS_ALIGNED(size, 4)) {
        STUB_LOGE("Flash read unaligned!\n");
        return STUB_LIB_ERR_FLASH_READ_UNALIGNED;
    }

    if (large_flash_mode) {
        return stub_target_flash_4byte_read(FLASH_SPI_NUM, addr, buffer, size);
    }

    return stub_target_flash_read_buff(addr, buffer, size);
}

int stub_lib_flash_write_buff(uint32_t addr, const void *buffer, uint32_t size, bool encrypt)
{
    STUB_LOG_TRACEF("addr: 0x%x, size: %u, large_flash_mode: %d, enc: %d\n", addr, size, large_flash_mode, encrypt);

    if (!IS_ALIGNED(addr, 4) || !IS_ALIGNED(size, 4)) {
        STUB_LOGE("Flash write unaligned!\n");
        return STUB_LIB_ERR_FLASH_WRITE_UNALIGNED;
    }

    if (large_flash_mode) {
        return stub_target_flash_4byte_write(FLASH_SPI_NUM, addr, buffer, size, encrypt);
    }

    return stub_target_flash_write_buff(addr, buffer, size, encrypt);
}

int stub_lib_flash_erase_chip(void)
{
    return stub_target_flash_erase_chip();
}

int stub_lib_flash_erase_sector(uint32_t addr)
{
    if (large_flash_mode) {
        return stub_target_flash_4byte_erase_sector(FLASH_SPI_NUM, addr);
    }

    return stub_target_flash_erase_sector(addr);
}

int stub_lib_flash_erase_block(uint32_t addr)
{
    if (large_flash_mode) {
        return stub_target_flash_4byte_erase_block(FLASH_SPI_NUM, addr);
    }

    return stub_target_flash_erase_block(addr);
}

int stub_lib_flash_wait_ready(uint64_t timeout_us)
{
    uint64_t elapsed_us = 0;
    do {
        if (!stub_target_flash_is_busy()) {
            return STUB_LIB_OK;
        }
        stub_lib_delay_us(1);
    } while (++elapsed_us < timeout_us);

    return STUB_LIB_ERR_TIMEOUT;
}

int stub_lib_flash_start_next_erase(uint32_t *next_erase_addr, uint32_t *remaining_size)
{
    if (next_erase_addr == NULL || remaining_size == NULL) {
        return STUB_LIB_ERR_INVALID_ARG;
    }

    if (stub_lib_flash_wait_ready(0) != STUB_LIB_OK) {
        return STUB_LIB_ERR_TIMEOUT;
    }

    if (*remaining_size == 0) {
        return STUB_LIB_OK;
    }

    uint32_t addr = *next_erase_addr;
    uint32_t remaining = *remaining_size;
    uint32_t erase_size;

    // Check if we can do a 64KB block erase
    bool block_erase = false;
    if (remaining >= STUB_FLASH_BLOCK_SIZE && (addr % STUB_FLASH_BLOCK_SIZE) == 0) {
        block_erase = true;
        erase_size = STUB_FLASH_BLOCK_SIZE;
    } else {
        erase_size = STUB_FLASH_SECTOR_SIZE;

        // Check sector alignment
        if (!IS_ALIGNED(addr, STUB_FLASH_SECTOR_SIZE)) {
            STUB_LOGE("Sector address 0x%x not aligned\n", addr);
            return STUB_LIB_ERR_INVALID_ARG;
        }
    }

    if (large_flash_mode) {
        /* Use 4-byte addressing for large flash */
        if (block_erase) {
            int res = stub_target_flash_4byte_erase_block_start(FLASH_SPI_NUM, addr);
            if (res != STUB_LIB_OK) {
                return res;
            }
        } else {
            int res = stub_target_flash_4byte_erase_sector_start(FLASH_SPI_NUM, addr);
            if (res != STUB_LIB_OK) {
                return res;
            }
        }
    } else {
        /* Use target-specific implementation for normal flash */
        if (block_erase) {
            stub_target_flash_erase_block_start(addr);
        } else {
            stub_target_flash_erase_sector_start(addr);
        }
    }

    // Update state
    *next_erase_addr = addr + erase_size;
    *remaining_size = (remaining >= erase_size) ? (remaining - erase_size) : 0;

    return STUB_LIB_OK;
}

int stub_lib_flash_erase_area(uint32_t addr, uint32_t size)
{
    if (!IS_ALIGNED(addr, STUB_FLASH_SECTOR_SIZE) || !IS_ALIGNED(size, STUB_FLASH_SECTOR_SIZE)) {
        STUB_LOGE("Erase area addr 0x%x or size 0x%x not sector-aligned\n", addr, size);
        return STUB_LIB_ERR_INVALID_ARG;
    }

    const uint32_t timeout_us = 1000000; // 1 second

    while (size > 0) {
        stub_lib_flash_start_next_erase(&addr, &size);
        if (stub_lib_flash_wait_ready(timeout_us) != STUB_LIB_OK) {
            STUB_LOGE("Erase area timeout at 0x%x, remaining %u\n", addr, size);
            return STUB_LIB_ERR_TIMEOUT;
        }
    }

    return STUB_LIB_OK;
}
