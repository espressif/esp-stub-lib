/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <stdbool.h>
#include <private/rom_flash.h>
#include <target/flash.h>
#include <esp-stub-lib/log.h>
#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/soc_utils.h>
#include <esp-stub-lib/err.h>
#include <soc/spi_mem_reg.h>

#define FLASH_SPI_NUM   1
#define STATUS_BUSY_BIT BIT(0)

extern esp_rom_spiflash_chip_t g_rom_flashchip;
extern uint32_t esp_rom_efuse_get_flash_gpio_info(void);
extern void esp_rom_spiflash_attach(uint32_t ishspi, bool legacy);

uint32_t stub_target_flash_get_spiconfig_efuse(void)
{
    return esp_rom_efuse_get_flash_gpio_info();
}

void stub_target_flash_attach(uint32_t ishspi, bool legacy)
{
    esp_rom_spiflash_attach(ishspi, legacy);
}

esp_rom_spiflash_chip_t *stub_target_flash_get_config(void)
{
    return &g_rom_flashchip;
}

static void spi_wait_ready(void)
{
    while (REG_GET_FIELD(SPI_MEM_FSM_REG(FLASH_SPI_NUM), SPI_MEM_ST)) {
        /* busy wait */
    }
    /* There is no HW arbiter on SPI0, so we need to wait for it to be ready */
    while ((REG_READ(SPI_MEM_FSM_REG(FLASH_SPI_NUM)) & SPI_MEM_ST)) {
        /* busy wait */
    }
}

bool stub_target_flash_is_busy(void)
{
    spi_wait_ready();

    REG_WRITE(SPI_MEM_RD_STATUS_REG(FLASH_SPI_NUM), 0);
    REG_WRITE(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_FLASH_RDSR);
    while (REG_READ(SPI_MEM_CMD_REG(FLASH_SPI_NUM)) != 0) {
    }
    uint32_t status_value = REG_READ(SPI_MEM_RD_STATUS_REG(FLASH_SPI_NUM));

    return (status_value & STATUS_BUSY_BIT) != 0;
}

void stub_target_flash_erase_sector_start(uint32_t addr)
{
    stub_target_flash_write_enable();
    spi_wait_ready();

    REG_WRITE(SPI_MEM_ADDR_REG(FLASH_SPI_NUM), addr & 0xffffff);
    REG_WRITE(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_FLASH_SE);
    while (REG_READ(SPI_MEM_CMD_REG(FLASH_SPI_NUM)) != 0) {
    }

    STUB_LOG_TRACEF("Started sector erase at 0x%x\n", addr);
}

void stub_target_flash_erase_block_start(uint32_t addr)
{
    stub_target_flash_write_enable();
    spi_wait_ready();

    REG_WRITE(SPI_MEM_ADDR_REG(FLASH_SPI_NUM), addr & 0xffffff);
    REG_WRITE(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_FLASH_BE);
    while (REG_READ(SPI_MEM_CMD_REG(FLASH_SPI_NUM)) != 0) {
    }

    STUB_LOG_TRACEF("Started block erase at 0x%x\n", addr);
}
