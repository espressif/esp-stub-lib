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

void stub_target_spi_wait_ready(void)
{
    while (REG_GET_FIELD(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_MST_ST) ||
           REG_GET_FIELD(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_SLV_ST)) {
        /* busy wait */
    }
}

uint8_t stub_target_flash_dout_clk_div(void)
{
    /* Sources 32, 20, 64 and 48 MHz, the ROM attach resets only the divider */
    return REG_GET_FIELD(PCR_MSPI_CONF_REG, PCR_MSPI_CLK_SEL) >= 2 ? 2 : 1;
}

bool stub_target_flash_needs_attach(void)
{
    return !stub_target_cache_is_enabled();
}
