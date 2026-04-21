/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>

#include <esp-stub-lib/soc_utils.h>

#include <target/flash.h>

#include <private/rom_flash.h>

#include <soc/spi_mem_compat.h>

#define SPI_INTERNAL 0

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

void stub_target_spi_wait_ready(void)
{
    while ((REG_READ(SPI_MEM_FSM_REG(FLASH_SPI_NUM)) & SPI_MEM_ST)) {
        /* busy wait */
    }

    /* There is no HW arbiter on SPI0, so we need to wait for it to be ready */
    while ((REG_READ(SPI_MEM_FSM_REG(SPI_INTERNAL)) & SPI_MEM_ST)) {
        /* busy wait */
    }
}

bool stub_target_flash_needs_attach(void)
{
    return (READ_PERI_REG(SPI_MEM_CACHE_FCTRL_REG(0)) & SPI_MEM_CACHE_FLASH_USR_CMD) == 0;
}
