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
#include <soc/spi1_mem_c_reg.h>

#define STATUS_BUSY_BIT BIT(0)

static void spi_wait_ready(void)
{
    while (REG_GET_FIELD(SPI1_MEM_C_CMD_REG, SPI1_MEM_C_MST_ST) ||
            REG_GET_FIELD(SPI1_MEM_C_CMD_REG, SPI1_MEM_C_SLV_ST)) {
        /* busy wait */
    }
}

bool stub_target_flash_is_busy(void)
{
    spi_wait_ready();

    REG_WRITE(SPI1_MEM_C_RD_STATUS_REG, 0);
    REG_WRITE(SPI1_MEM_C_CMD_REG, SPI1_MEM_C_FLASH_RDSR);
    while (REG_READ(SPI1_MEM_C_CMD_REG) != 0) { }
    uint32_t status_value = REG_READ(SPI1_MEM_C_RD_STATUS_REG);

    return (status_value & STATUS_BUSY_BIT) != 0;
}

void stub_target_flash_erase_sector_start(uint32_t addr)
{
    stub_target_flash_write_enable();
    spi_wait_ready();

    REG_WRITE(SPI1_MEM_C_ADDR_REG, addr & 0xffffff);
    REG_WRITE(SPI1_MEM_C_CMD_REG, SPI1_MEM_C_FLASH_SE);
    while (REG_READ(SPI1_MEM_C_CMD_REG) != 0) { }

    STUB_LOG_TRACEF("Started sector erase at 0x%x\n", addr);
}

void stub_target_flash_erase_block_start(uint32_t addr)
{
    stub_target_flash_write_enable();
    spi_wait_ready();

    REG_WRITE(SPI1_MEM_C_ADDR_REG, addr & 0xffffff);
    REG_WRITE(SPI1_MEM_C_CMD_REG, SPI1_MEM_C_FLASH_BE);
    while (REG_READ(SPI1_MEM_C_CMD_REG) != 0) { }

    STUB_LOG_TRACEF("Started block erase at 0x%x\n", addr);
}
