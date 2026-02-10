/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <stddef.h>
#include <log.h>
#include <target/flash.h>
#include <private/rom_flash.h>
#include <private/rom_flash_config.h>
#include <esp-stub-lib/err.h>

extern esp_rom_spiflash_result_t esp_rom_spiflash_erase_chip(void);
extern void esp_rom_spiflash_attach(uint32_t ishspi, bool legacy);
extern esp_rom_spiflash_result_t esp_rom_spiflash_write(uint32_t flash_addr, const void *data, uint32_t size);
extern esp_rom_spiflash_result_t esp_rom_spiflash_write_encrypted(uint32_t flash_addr, const void *data, uint32_t size);
extern esp_rom_spiflash_result_t esp_rom_spiflash_erase_sector(uint32_t addr);
extern esp_rom_spiflash_result_t esp_rom_spiflash_erase_block(uint32_t addr);
extern esp_rom_spiflash_result_t esp_rom_spiflash_erase_area(uint32_t addr, uint32_t size);
extern esp_rom_spiflash_result_t esp_rom_spiflash_write_enable(struct esp_rom_spiflash_chip *flash_chip);
extern void esp_rom_spiflash_write_encrypted_enable(void);
extern void esp_rom_spiflash_write_encrypted_disable(void);
extern esp_rom_spiflash_result_t esp_rom_spiflash_unlock(void);

int stub_target_flash_update_config(uint32_t flash_id,
                                    uint32_t flash_size,
                                    uint32_t block_size,
                                    uint32_t sector_size,
                                    uint32_t page_size,
                                    uint32_t status_mask)
{
    esp_rom_spiflash_result_t res =
        esp_rom_spiflash_config_param(flash_id, flash_size, block_size, sector_size, page_size, status_mask);
    if (res != ESP_ROM_SPIFLASH_RESULT_OK) {
        STUB_LOGE("Failed to update flash config: %d\n", res);
        return STUB_LIB_FAIL;
    }
    return STUB_LIB_OK;
}

uint32_t stub_target_flash_id_to_flash_size(uint32_t flash_id)
{
    const uint32_t id = flash_id & 0xff;
    switch (id) {
    case 0x12: // 256 KB
    case 0x13:
    case 0x14: // 1 MB
    case 0x15:
    case 0x16:
    case 0x17:
    case 0x18:
    case 0x19: // 32 MB
    case 0x1A: // 64 MB
    case 0x1B: // 128 MB
    case 0x1C: // 256 MB
        return 1u << id;
    case 0x39:
        return 32 * 1024 * 1024;
    }

    STUB_LOGE("Unknown flash_id: 0x%x\n", flash_id);
    return 0;
}

void __attribute__((weak)) stub_target_flash_init(void *state)
{
    (void)state;
    esp_rom_spiflash_attach(0, 0);
}

struct esp_rom_spiflash_chip *__attribute__((weak)) stub_target_flash_get_config(void)
{
    extern esp_rom_spiflash_legacy_data_t *rom_spiflash_legacy_data;
    return &rom_spiflash_legacy_data->chip;
}

uint32_t __attribute__((weak)) stub_target_flash_get_flash_id(void)
{
    esp_rom_spi_flash_update_id();
    return stub_target_flash_get_config()->flash_id;
}

void __attribute__((weak)) stub_target_flash_deinit(const void *state)
{
    (void)state;
}

void stub_target_flash_write_enable(void)
{
    struct esp_rom_spiflash_chip *chip = stub_target_flash_get_config();
    esp_rom_spiflash_write_enable(chip);
}

int __attribute__((weak)) stub_target_flash_write_buff(uint32_t addr, const void *buffer, uint32_t size, bool encrypt)
{
    esp_rom_spiflash_result_t res;
    if (encrypt) {
        res = esp_rom_spiflash_write_encrypted(addr, buffer, size);
    } else {
        res = esp_rom_spiflash_write(addr, buffer, size);
    }
    STUB_LOG_TRACEF("(0x%x, 0x%x, %u, %d) results: %d\n", addr, (uint32_t)buffer, size, encrypt, res);
    return res == ESP_ROM_SPIFLASH_RESULT_OK ? STUB_LIB_OK : STUB_LIB_FAIL;
}

int __attribute__((weak)) stub_target_flash_read_buff(uint32_t addr, void *buffer, uint32_t size)
{
    esp_rom_spiflash_result_t res = esp_rom_spiflash_read(addr, (uint32_t *)buffer, (int32_t)size);
    STUB_LOG_TRACEF("esp_rom_spiflash_read(0x%x, 0x%x, %u) results: %d\n", addr, (uint32_t)buffer, size, res);
    if (res != ESP_ROM_SPIFLASH_RESULT_OK) {
        return STUB_LIB_ERR_FLASH_READ_ROM_ERR;
    }
    return STUB_LIB_OK;
}

int stub_target_flash_erase_chip(void)
{
    if (esp_rom_spiflash_erase_chip() == ESP_ROM_SPIFLASH_RESULT_OK) {
        return STUB_LIB_OK;
    }
    return STUB_LIB_FAIL;
}

int stub_target_flash_erase_sector(uint32_t addr)
{
    if (esp_rom_spiflash_erase_sector(addr) == ESP_ROM_SPIFLASH_RESULT_OK) {
        return STUB_LIB_OK;
    }
    return STUB_LIB_FAIL;
}

int stub_target_flash_erase_block(uint32_t addr)
{
    if (esp_rom_spiflash_erase_block(addr) == ESP_ROM_SPIFLASH_RESULT_OK) {
        return STUB_LIB_OK;
    }
    return STUB_LIB_FAIL;
}

int stub_target_flash_erase_area(uint32_t addr, uint32_t size)
{
    if (esp_rom_spiflash_erase_area(addr, size) == ESP_ROM_SPIFLASH_RESULT_OK) {
        return STUB_LIB_OK;
    }
    return STUB_LIB_FAIL;
}

void stub_target_flash_attach(uint32_t ishspi, bool legacy)
{
    esp_rom_spiflash_attach(ishspi, legacy);
}

void __attribute__((weak)) stub_target_opiflash_exec_cmd(const opiflash_cmd_params_t *params)
{
    /* Empty implementation for targets without large flash support */
    (void)params;
}

void __attribute__((weak)) stub_target_flash_write_encrypted_enable(void)
{
    esp_rom_spiflash_write_encrypted_enable();
}

void __attribute__((weak)) stub_target_flash_write_encrypted_disable(void)
{
    esp_rom_spiflash_write_encrypted_disable();
}

int stub_target_flash_unlock(void)
{
    if (esp_rom_spiflash_unlock() == ESP_ROM_SPIFLASH_RESULT_OK) {
        return STUB_LIB_OK;
    }
    return STUB_LIB_FAIL;
}
