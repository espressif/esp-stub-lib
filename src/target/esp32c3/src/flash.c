/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>

#include <esp-stub-lib/soc_utils.h>

#include <target/flash.h>

#include <soc/spi_mem_compat.h>

#define SPI_INTERNAL 0

extern uint32_t esp_rom_efuse_get_flash_gpio_info(void);

uint32_t stub_target_flash_get_spiconfig_efuse(void)
{
    return esp_rom_efuse_get_flash_gpio_info();
}

/* C3 ROM legacy_funcs starts at read_sub_len; it has no se/be_addr_bit_len. */
uint8_t stub_target_flash_erase_addr_bits(bool block)
{
    (void)block;
    return 24;
}

void stub_target_spi_wait_ready(void)
{
    /* There is no HW arbiter on SPI0, so we need to wait for it to be ready */
    while (REG_GET_FIELD(SPI_MEM_FSM_REG(SPI_INTERNAL), SPI_MEM_EM_ST)) {
        /* busy wait */
    }
    while (REG_GET_FIELD(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_MST_ST)) {
        /* busy wait */
    }
}
