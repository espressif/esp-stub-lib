/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <stdbool.h>
#include <esp-stub-lib/log.h>
#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/soc_utils.h>
#include <esp-stub-lib/err.h>
#include <soc/spi_mem_reg.h>

#define SPI_NUM 1
#define STATUS_BUSY_BIT BIT(0)

static void spi_wait_ready(void)
{
    while (REG_GET_FIELD(SPI_MEM_FSM_REG(SPI_NUM), SPI_MEM_ST)) {
        /* busy wait */
    }
}

bool stub_target_flash_is_busy(void)
{
    spi_wait_ready();

    REG_WRITE(SPI_MEM_RD_STATUS_REG(SPI_NUM), 0);
    REG_WRITE(SPI_MEM_CMD_REG(SPI_NUM), SPI_MEM_FLASH_RDSR);
    while (REG_READ(SPI_MEM_CMD_REG(SPI_NUM)) != 0) { }
    uint32_t status_value = REG_READ(SPI_MEM_RD_STATUS_REG(SPI_NUM));

    return (status_value & STATUS_BUSY_BIT) != 0;
}

static void spi_write_enable(void)
{
    while (stub_target_flash_is_busy()) { }

    REG_WRITE(SPI_MEM_CMD_REG(SPI_NUM), SPI_MEM_FLASH_WREN);
    while (REG_READ(SPI_MEM_CMD_REG(SPI_NUM)) != 0) { }
}

void stub_target_flash_erase_sector_start(uint32_t addr)
{
    spi_write_enable();
    spi_wait_ready();

    REG_WRITE(SPI_MEM_ADDR_REG(SPI_NUM), addr & 0xffffff);
    REG_WRITE(SPI_MEM_CMD_REG(SPI_NUM), SPI_MEM_FLASH_SE);
    while (REG_READ(SPI_MEM_CMD_REG(SPI_NUM)) != 0) { }

    STUB_LOG_TRACEF("Started sector erase at 0x%x\n", addr);
}

void stub_target_flash_erase_block_start(uint32_t addr)
{
    spi_write_enable();
    spi_wait_ready();

    REG_WRITE(SPI_MEM_ADDR_REG(SPI_NUM), addr & 0xffffff);
    REG_WRITE(SPI_MEM_CMD_REG(SPI_NUM), SPI_MEM_FLASH_BE);
    while (REG_READ(SPI_MEM_CMD_REG(SPI_NUM)) != 0) { }

    STUB_LOG_TRACEF("Started block erase at 0x%x\n", addr);
}
