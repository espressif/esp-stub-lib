/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <stdbool.h>
#include <target/flash.h>
#include <private/rom_flash.h>
#include <esp-stub-lib/log.h>
#include <esp-stub-lib/err.h>
#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/soc_utils.h>

// ESP8266 uses SPI0 as the flash interface
#define SPI_BASE_REG      0x60000200
#define SPI_EXT2_REG      (SPI_BASE_REG + 0xF8)
#define SPI_CMD_REG       (SPI_BASE_REG + 0x00)
#define SPI_RD_STATUS_REG (SPI_BASE_REG + 0x10)
#define SPI_ADDR_REG      (SPI_BASE_REG + 0x04)
#define SPI_FLASH_WREN    BIT(30)
#define SPI_FLASH_RDID    BIT(28)
#define SPI_FLASH_RDSR    BIT(27)
#define SPI_FLASH_SE      BIT(24)
#define SPI_FLASH_BE      BIT(23)
#define SPI_W0            (SPI_BASE_REG + 0x40)
#define SPI_CMD           (SPI_BASE_REG + 0x0)

#define SPI_ST            0x00000007
#define SPI_ST_M          ((SPI_ST_V) << (SPI_ST_S))
#define SPI_ST_V          0x7
#define SPI_ST_S          0

#define STATUS_BUSY_BIT   BIT(0)

extern esp_rom_spiflash_chip_t g_rom_flashchip;
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
    return &g_rom_flashchip;
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
    STUB_LOG_TRACEF("(0x%x, 0x%x, %u, %d) results: %d\n", addr, (uint32_t)buffer, size, encrypt, res);
    return res == ESP_ROM_SPIFLASH_RESULT_OK ? STUB_LIB_OK : STUB_LIB_FAIL;
}

static void spi_wait_ready(void)
{
    while (REG_GET_FIELD(SPI_EXT2_REG, SPI_ST)) {
        /* busy wait */
    }
}

bool stub_target_flash_is_busy(void)
{
    spi_wait_ready();

    REG_WRITE(SPI_RD_STATUS_REG, 0);
    REG_WRITE(SPI_CMD_REG, SPI_FLASH_RDSR);
    while (REG_READ(SPI_CMD_REG) != 0) {
    }
    uint32_t status_value = REG_READ(SPI_RD_STATUS_REG);

    return (status_value & STATUS_BUSY_BIT) != 0;
}

void stub_target_flash_write_enable(void)
{
    // ROM SPI_write_enable() does not work for ESP8266, so we use our own implementation
    spi_wait_ready();
    REG_WRITE(SPI_CMD_REG, SPI_FLASH_WREN);
    while (REG_READ(SPI_CMD_REG) != 0)
        ;
}

void stub_target_flash_erase_sector_start(uint32_t addr)
{
    stub_target_flash_write_enable();
    spi_wait_ready();

    REG_WRITE(SPI_ADDR_REG, addr & 0xffffff);
    REG_WRITE(SPI_CMD_REG, SPI_FLASH_SE);
    while (REG_READ(SPI_CMD_REG) != 0) {
    }

    STUB_LOG_TRACEF("Started sector erase at 0x%x\n", addr);
}

void stub_target_flash_erase_block_start(uint32_t addr)
{
    stub_target_flash_write_enable();
    spi_wait_ready();

    REG_WRITE(SPI_ADDR_REG, addr & 0xffffff);
    REG_WRITE(SPI_CMD_REG, SPI_FLASH_BE);
    while (REG_READ(SPI_CMD_REG) != 0) {
    }

    STUB_LOG_TRACEF("Started block erase at 0x%x\n", addr);
}

void stub_target_flash_write_encrypted_enable(void)
{
    // Empty weak function for targets that does not support AES-XTS
}

void stub_target_flash_write_encrypted_disable(void)
{
    // Empty weak function for targets that does not support AES-XTS
}
