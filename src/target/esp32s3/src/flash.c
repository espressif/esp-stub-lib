/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <log.h>
#include <err.h>
#include <target/flash.h>
#include <private/rom_flash.h>
#include <private/rom_flash_config.h>
#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/soc_utils.h>
#include <soc/spi_mem_reg.h>

#define SPI_NUM 1
#define STATUS_BUSY_BIT BIT(0)

extern uint32_t ets_efuse_get_spiconfig(void);
extern esp_rom_spiflash_legacy_funcs_t *rom_spiflash_legacy_funcs;

static void stub_target_flash_init_funcs(void)
{
    static esp_rom_spiflash_legacy_funcs_t funcs = {
        .se_addr_bit_len = 24,
        .be_addr_bit_len = 24,
        .pp_addr_bit_len = 24,
        .rd_addr_bit_len = 24,
        .read_sub_len = 16, // 32 - from IDF
        .write_sub_len = 32,
    };
    rom_spiflash_legacy_funcs = &funcs;
}

void stub_target_flash_init(void *state)
{
    (void)state;
    uint32_t spiconfig = ets_efuse_get_spiconfig();
    esp_rom_spiflash_attach(spiconfig, 0);
    if (ets_efuse_flash_octal_mode()) {
        STUB_LOGD("octal mode is on\n");
        stub_target_flash_init_funcs();
    }
}

int stub_target_flash_read_buff(uint32_t addr, void *buffer, uint32_t size)
{
    esp_rom_spiflash_result_t res = esp_rom_spiflash_read(addr, (uint32_t *)buffer, (int32_t)size);
    STUB_LOG_TRACEF("esp_rom_spiflash_read(0x%x, 0x%x, %u) results: %d\n", addr, (uint32_t)buffer, size, res);
    if (res != ESP_ROM_SPIFLASH_RESULT_OK) {
        return STUB_LIB_ERR_FLASH_READ_ROM_ERR;
    }
    return STUB_LIB_OK;
}

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

void stub_target_flash_erase_sector_start(uint32_t addr)
{
    stub_target_flash_write_enable();
    spi_wait_ready();

    REG_WRITE(SPI_MEM_ADDR_REG(SPI_NUM), addr & 0xffffff);
    REG_WRITE(SPI_MEM_CMD_REG(SPI_NUM), SPI_MEM_FLASH_SE);
    while (REG_READ(SPI_MEM_CMD_REG(SPI_NUM)) != 0) { }

    STUB_LOG_TRACEF("Started sector erase at 0x%x\n", addr);
}

void stub_target_flash_erase_block_start(uint32_t addr)
{
    stub_target_flash_write_enable();
    spi_wait_ready();

    REG_WRITE(SPI_MEM_ADDR_REG(SPI_NUM), addr & 0xffffff);
    REG_WRITE(SPI_MEM_CMD_REG(SPI_NUM), SPI_MEM_FLASH_BE);
    while (REG_READ(SPI_MEM_CMD_REG(SPI_NUM)) != 0) { }

    STUB_LOG_TRACEF("Started block erase at 0x%x\n", addr);
}
