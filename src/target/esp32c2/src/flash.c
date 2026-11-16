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

void stub_target_spi_wait_ready(void)
{
    while (REG_GET_FIELD(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_MST_ST)) {
        /* busy wait */
    }
}

uint32_t stub_target_flash_get_src_freq_mhz(void)
{
    /* ESP-IDF uses 60 MHz with the PLL system clock. With the XTAL system clock
     * the source is not confirmed, and 80 MHz keeps the flash at 40 MHz or below. */
    return REG_GET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_SOC_CLK_SEL) == 1 ? 60 : 80;
}
