/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <log.h>
#include <err.h>

#include <target/flash.h>
#include <private/rom_flash.h>
#include <private/rom_flash_config.h>

/**
  * @brief Read SPI flash pin configuration from eFuse
  *
  * Available only for ESP32, S2, C3, S3
  *
  * @return Configuration value:
  * - 0 for default SPI pins
  * - other value for custom configuration
  */
extern uint32_t ets_efuse_get_spiconfig(void);

static void stub_target_flash_init_funcs(void)
{
    static esp_rom_spiflash_legacy_funcs_t funcs = {
        .se_addr_bit_len = 24,
        .be_addr_bit_len = 24,
        .pp_addr_bit_len = 24,
        .rd_addr_bit_len = 24,
        .read_sub_len = 16, // 32 - from IDF
        .write_sub_len = 32,
    };
    rom_spiflash_legacy_funcs = &funcs;
}

void stub_target_flash_init(void *state)
{
    (void)state;
    uint32_t spiconfig = ets_efuse_get_spiconfig();
    esp_rom_spiflash_attach(spiconfig, 0);
    if (ets_efuse_flash_octal_mode()) {
        STUB_LOGD("octal mode is on\n");
        stub_target_flash_init_funcs();
    }
}

const struct esp_rom_spiflash_chip *stub_target_flash_get_config(void)
{
    return &g_rom_flashchip;
}

uint32_t stub_target_flash_get_flash_id(void)
{
    esp_rom_spi_flash_update_id();
    return stub_target_flash_get_config()->flash_id;
}

int stub_target_flash_read_buff(uint32_t addr, void *buffer, uint32_t size)
{
    if (addr & 3 || size & 3) {
        STUB_LOGE("Unaligned read: 0x%x, %u\n", addr, size);
        return STUB_LIB_ERR_FLASH_READ_UNALIGNED;
    }
    esp_rom_spiflash_result_t res = esp_rom_spiflash_read(addr, (uint32_t*)buffer, (int32_t)size);
    STUB_LOG_TRACEF("esp_rom_spiflash_read(0x%x, 0x%x, %u) results: %d\n", addr, (uint32_t)buffer, size, res);
    if (res != ESP_ROM_SPIFLASH_RESULT_OK) {
        return STUB_LIB_ERR_FLASH_READ_ROM_ERR;
    }
    return STUB_LIB_OK;
}
