/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <stdbool.h>
#include <target/flash.h>
#include <esp-stub-lib/log.h>
#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/soc_utils.h>
#include <esp-stub-lib/err.h>
#include <soc/spi_reg.h>
#include <private/rom_flash.h>

#define SPI_NUM         1
#define STATUS_BUSY_BIT BIT(0)

extern struct esp_rom_spiflash_chip g_rom_flashchip;

struct esp_rom_spiflash_chip *stub_target_flash_get_config(void)
{
    return &g_rom_flashchip;
}

static uint32_t get_flash_id(void)
{
    WRITE_PERI_REG(SPI_W0_REG(SPI_NUM), 0); // clear register
    WRITE_PERI_REG(SPI_CMD_REG(SPI_NUM), SPI_FLASH_RDID);
    while (READ_PERI_REG(SPI_CMD_REG(SPI_NUM)) != 0)
        ;
    return REG_READ(SPI_W0_REG(SPI_NUM)) & 0xffffff;
}

uint32_t stub_target_flash_get_flash_id(void)
{
    struct esp_rom_spiflash_chip *chip = stub_target_flash_get_config();
    chip->flash_id = get_flash_id();
    return chip->flash_id;
}

static void spi_wait_ready(void)
{
    // No need to wait for SPI0 as there is a hardware arbiter between SPI0 and SPI1
    while (REG_GET_FIELD(SPI_EXT2_REG(SPI_NUM), SPI_ST)) {
        /* busy wait */
    }
}

bool stub_target_flash_is_busy(void)
{
    spi_wait_ready();

    REG_WRITE(SPI_RD_STATUS_REG(SPI_NUM), 0);
    REG_WRITE(SPI_CMD_REG(SPI_NUM), SPI_FLASH_RDSR);
    while (REG_READ(SPI_CMD_REG(SPI_NUM)) != 0) {
    }
    uint32_t status_value = REG_READ(SPI_RD_STATUS_REG(SPI_NUM));

    return (status_value & STATUS_BUSY_BIT) != 0;
}

void stub_target_flash_erase_sector_start(uint32_t addr)
{
    stub_target_flash_write_enable();
    spi_wait_ready();

    REG_WRITE(SPI_ADDR_REG(SPI_NUM), addr & 0xffffff);
    REG_WRITE(SPI_CMD_REG(SPI_NUM), SPI_FLASH_SE);
    while (REG_READ(SPI_CMD_REG(SPI_NUM)) != 0) {
    }

    STUB_LOG_TRACEF("Started sector erase at 0x%x\n", addr);
}

void stub_target_flash_erase_block_start(uint32_t addr)
{
    stub_target_flash_write_enable();
    spi_wait_ready();

    REG_WRITE(SPI_ADDR_REG(SPI_NUM), addr & 0xffffff);
    REG_WRITE(SPI_CMD_REG(SPI_NUM), SPI_FLASH_BE);
    while (REG_READ(SPI_CMD_REG(SPI_NUM)) != 0) {
    }

    STUB_LOG_TRACEF("Started block erase at 0x%x\n", addr);
}
