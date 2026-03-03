/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>

#include <esp-stub-lib/err.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/flash.h>

#include <private/rom_flash.h>

#include <soc/spi_mem_compat.h>

// ESP8266 contains g_rom_flashchip, but it is in different order than esp_rom_spiflash_chip_t,
// esp_rom_spiflash_config_param might take also different order of parameters but due to missing
// possibility to check in ROM code custom implementation seems like okay solution preventing
// possible future issues.
static esp_rom_spiflash_chip_t g_rom_flashchip_custom = {
    .block_size = STUB_FLASH_BLOCK_SIZE,
    .sector_size = STUB_FLASH_SECTOR_SIZE,
    .page_size = STUB_FLASH_PAGE_SIZE,
    .status_mask = STUB_FLASH_STATUS_MASK,
};

extern int esp_rom_spiflash_write(uint32_t flash_addr, const void *data, uint32_t size);
extern void esp_rom_spiflash_attach(void);
extern void esp_rom_spiflash_select_padsfunc(void);

void stub_target_reset_default_spi_pins(void)
{
    /* ESP8266 doesn't support GPIO matrix routing for SPI flash.
     * It only supports HSPI, but that's not currently implemented.
     * The default SPI pins are always used through IOMUX.
     * See: https://github.com/espressif/esptool/issues/98 */
}

esp_rom_spiflash_chip_t *stub_target_flash_get_config(void)
{
    return &g_rom_flashchip_custom;
}

int stub_target_flash_update_config(uint32_t flash_id,
                                    uint32_t flash_size,
                                    uint32_t block_size,
                                    uint32_t sector_size,
                                    uint32_t page_size,
                                    uint32_t status_mask)
{
    g_rom_flashchip_custom.flash_id = flash_id;
    g_rom_flashchip_custom.chip_size = flash_size;
    g_rom_flashchip_custom.block_size = block_size;
    g_rom_flashchip_custom.sector_size = sector_size;
    g_rom_flashchip_custom.page_size = page_size;
    g_rom_flashchip_custom.status_mask = status_mask;
    return STUB_LIB_OK;
}

void stub_target_flash_attach(uint32_t ishspi, bool legacy)
{
    (void)ishspi;
    (void)legacy;
    esp_rom_spiflash_select_padsfunc();
    esp_rom_spiflash_attach();
}

uint32_t stub_target_flash_get_flash_id(void)
{
    REG_WRITE(SPI_W0, 0);
    REG_WRITE(SPI_CMD, SPI_FLASH_RDID);
    while (REG_READ(SPI_CMD) != 0) {
    }
    return (REG_READ(SPI_W0) & 0xffffff) >> 16;
}

int stub_target_flash_write_buff(uint32_t addr, const void *buffer, uint32_t size, bool encrypt)
{
    if (encrypt) {
        return STUB_LIB_ERR_NOT_SUPPORTED;
    }
    int res = esp_rom_spiflash_write(addr, buffer, size);
    return res == ESP_ROM_SPIFLASH_RESULT_OK ? STUB_LIB_OK : STUB_LIB_FAIL;
}

void stub_target_spi_wait_ready(void)
{
    while (REG_GET_FIELD(SPI_EXT2_REG, SPI_ST)) {
        /* busy wait */
    }
}

void stub_target_flash_write_enable(void)
{
    // ROM SPI_write_enable() does not work for ESP8266, so we use our own implementation
    stub_target_spi_wait_ready();
    REG_WRITE(SPI_CMD_REG, SPI_FLASH_WREN);
    while (REG_READ(SPI_CMD_REG) != 0)
        ;
}

void stub_target_flash_write_encrypted_enable(void)
{
    // Empty weak function for targets that does not support AES-XTS
}

void stub_target_flash_write_encrypted_disable(void)
{
    // Empty weak function for targets that does not support AES-XTS
}
