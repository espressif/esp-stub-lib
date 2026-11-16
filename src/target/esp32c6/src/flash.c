/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>

#include <esp-stub-lib/log.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>
#include <target/flash.h>

#include <soc/pcr_reg.h>
#include <soc/spi_mem_compat.h>

extern void esp_rom_spiflash_attach(uint32_t ishspi, bool legacy);
extern void stub_target_clock_restore_mspi(void);

void stub_target_spi_wait_ready(void)
{
    while (REG_GET_FIELD(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_MST_ST) ||
           REG_GET_FIELD(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_SLV_ST)) {
        /* busy wait */
    }
}

bool stub_target_flash_needs_attach(void)
{
    return !stub_target_cache_is_enabled();
}

void stub_target_flash_init(void *state, stub_lib_flash_attach_policy_t attach_policy)
{
    (void)state;

    if (attach_policy == STUB_LIB_FLASH_ATTACH_ALWAYS || stub_target_flash_needs_attach()) {
        STUB_LOGD("Attach spi flash...\n");
        stub_target_flash_attach(0, false);
    }
}

void stub_target_flash_attach(uint32_t ishspi, bool legacy)
{
    esp_rom_spiflash_attach(ishspi, legacy);
    stub_target_clock_restore_mspi();
}

uint32_t stub_target_flash_get_src_freq_mhz(void)
{
    /* The 480 MHz PLL with the PLL system clock, otherwise the 40 MHz XTAL */
    if (REG_GET_FIELD(PCR_SYSCLK_CONF_REG, PCR_SOC_CLK_SEL) == 1) {
        return 480 / (REG_GET_FIELD(PCR_MSPI_CLK_CONF_REG, PCR_MSPI_FAST_HS_DIV_NUM) + 1);
    }
    return 40 / (REG_GET_FIELD(PCR_MSPI_CLK_CONF_REG, PCR_MSPI_FAST_LS_DIV_NUM) + 1);
}
