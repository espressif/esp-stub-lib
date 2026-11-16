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
#include <soc/system_reg.h>

#define SPI_INTERNAL 0

/* The ESP32-C3 ROM struct starts at read_sub_len, without the address bit lengths of the newer chips */
typedef struct {
    uint32_t read_sub_len;
    uint32_t write_sub_len;
} esp32c3_rom_spiflash_legacy_funcs_t;

extern esp32c3_rom_spiflash_legacy_funcs_t *esp32c3_rom_spiflash_legacy_funcs;
extern uint32_t esp_rom_efuse_get_flash_gpio_info(void);

uint32_t stub_target_flash_get_spiconfig_efuse(void)
{
    return esp_rom_efuse_get_flash_gpio_info();
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

void stub_target_flash_set_rom_bytes_per_cmd(uint32_t bytes)
{
    esp32c3_rom_spiflash_legacy_funcs->read_sub_len = bytes;
    esp32c3_rom_spiflash_legacy_funcs->write_sub_len = bytes;
}

uint32_t stub_target_flash_get_src_freq_mhz(void)
{
    /* Selected from the 480 MHz PLL with the PLL system clock, the unused value as the fastest source.
     * With the XTAL system clock the source is not confirmed, and 80 MHz keeps the flash at 40 MHz or below. */
    static const uint16_t src_mhz[] = { 80, 120, 160, 160 };
    if (REG_GET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_SOC_CLK_SEL) != 1) {
        return 80;
    }
    return src_mhz[REG_GET_FIELD(SPI_MEM_CORE_CLK_SEL_REG(0), SPI_MEM_SPI01_CLK_SEL)];
}
