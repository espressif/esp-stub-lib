/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <stdbool.h>
#include <target/flash.h>
#include <esp-stub-lib/soc_utils.h>
#include <soc/spi_mem_compat.h>
#include <private/rom_flash.h>

extern esp_rom_spiflash_chip_t g_rom_flashchip;
extern uint32_t esp_rom_efuse_get_flash_gpio_info(void);

uint32_t stub_target_flash_get_spiconfig_efuse(void)
{
    return esp_rom_efuse_get_flash_gpio_info();
}

esp_rom_spiflash_chip_t *stub_target_flash_get_config(void)
{
    return &g_rom_flashchip;
}

uint32_t stub_target_flash_get_flash_id(void)
{
    WRITE_PERI_REG(SPI_W0_REG(FLASH_SPI_NUM), 0); // clear register
    WRITE_PERI_REG(SPI_CMD_REG(FLASH_SPI_NUM), SPI_FLASH_RDID);
    while (READ_PERI_REG(SPI_CMD_REG(FLASH_SPI_NUM)) != 0) {
        /* busy wait */
    }
    return (REG_READ(SPI_W0_REG(FLASH_SPI_NUM)) & 0xffffff) >> 16;
}

void stub_target_spi_wait_ready(void)
{
    // No need to wait for SPI0 as there is a hardware arbiter between SPI0 and SPI1
    while (REG_GET_FIELD(SPI_EXT2_REG(FLASH_SPI_NUM), SPI_ST)) {
        /* busy wait */
    }
}
