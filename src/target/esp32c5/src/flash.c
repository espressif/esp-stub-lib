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
#include <private/rom_flash.h>
#include <soc/spi1_mem_reg.h>

#define SPI_NUM         1
#define STATUS_BUSY_BIT BIT(0)

/* ECO version from ROM - used to route to correct ROM functions */
extern uint32_t _rom_eco_version;
extern void esp_rom_spiflash_attach(uint32_t ishspi, bool legacy);

void stub_target_flash_attach(uint32_t ishspi, bool legacy)
{
    esp_rom_spiflash_attach(ishspi, legacy);
}

/* ECO-specific ROM function declarations */
extern void esp_rom_opiflash_exec_cmd_eco2(int spi_num,
                                           spi_flash_mode_t mode,
                                           uint32_t cmd,
                                           int cmd_bit_len,
                                           uint32_t addr,
                                           int addr_bit_len,
                                           int dummy_bits,
                                           const uint8_t *mosi_data,
                                           int mosi_bit_len,
                                           uint8_t *miso_data,
                                           int miso_bit_len,
                                           uint32_t cs_mask,
                                           bool is_write_erase_operation);

extern void esp_rom_opiflash_exec_cmd_eco3(int spi_num,
                                           spi_flash_mode_t mode,
                                           uint32_t cmd,
                                           int cmd_bit_len,
                                           uint32_t addr,
                                           int addr_bit_len,
                                           int dummy_bits,
                                           const uint8_t *mosi_data,
                                           int mosi_bit_len,
                                           uint8_t *miso_data,
                                           int miso_bit_len,
                                           uint32_t cs_mask,
                                           bool is_write_erase_operation);

void stub_target_opiflash_exec_cmd(const opiflash_cmd_params_t *params)
{
    if (_rom_eco_version >= 3) {
        esp_rom_opiflash_exec_cmd_eco3(params->spi_num,
                                       params->mode,
                                       params->cmd,
                                       params->cmd_bit_len,
                                       params->addr,
                                       params->addr_bit_len,
                                       params->dummy_bits,
                                       params->mosi_data,
                                       params->mosi_bit_len,
                                       params->miso_data,
                                       params->miso_bit_len,
                                       params->cs_mask,
                                       params->is_write_erase_operation);
    } else {
        esp_rom_opiflash_exec_cmd_eco2(params->spi_num,
                                       params->mode,
                                       params->cmd,
                                       params->cmd_bit_len,
                                       params->addr,
                                       params->addr_bit_len,
                                       params->dummy_bits,
                                       params->mosi_data,
                                       params->mosi_bit_len,
                                       params->miso_data,
                                       params->miso_bit_len,
                                       params->cs_mask,
                                       params->is_write_erase_operation);
    }
}

static void spi_wait_ready(void)
{
    while (REG_GET_FIELD(SPI_MEM_CMD_REG(SPI_NUM), SPI_MEM_MST_ST)) {
        /* busy wait */
    }
}

bool stub_target_flash_is_busy(void)
{
    spi_wait_ready();

    REG_WRITE(SPI_MEM_RD_STATUS_REG(SPI_NUM), 0);
    REG_WRITE(SPI_MEM_CMD_REG(SPI_NUM), SPI_MEM_FLASH_RDSR);
    while (REG_READ(SPI_MEM_CMD_REG(SPI_NUM)) != 0) {
    }
    uint32_t status_value = REG_READ(SPI_MEM_RD_STATUS_REG(SPI_NUM));

    return (status_value & STATUS_BUSY_BIT) != 0;
}

void stub_target_flash_erase_sector_start(uint32_t addr)
{
    stub_target_flash_write_enable();
    spi_wait_ready();
    REG_WRITE(SPI_MEM_ADDR_REG(SPI_NUM), addr & 0xffffff);
    REG_WRITE(SPI_MEM_CMD_REG(SPI_NUM), SPI_MEM_FLASH_SE);
    while (REG_READ(SPI_MEM_CMD_REG(SPI_NUM)) != 0) {
    }

    STUB_LOG_TRACEF("Started sector erase at 0x%x\n", addr);
}

void stub_target_flash_erase_block_start(uint32_t addr)
{
    stub_target_flash_write_enable();
    spi_wait_ready();
    REG_WRITE(SPI_MEM_ADDR_REG(SPI_NUM), addr & 0xffffff);
    REG_WRITE(SPI_MEM_CMD_REG(SPI_NUM), SPI_MEM_FLASH_BE);
    while (REG_READ(SPI_MEM_CMD_REG(SPI_NUM)) != 0) {
    }

    STUB_LOG_TRACEF("Started block erase at 0x%x\n", addr);
}

uint32_t stub_target_get_max_supported_flash_size(void)
{
    /* ESP32-C5 supports up to 32MB with 4-byte addressing */
    return 32 * 1024 * 1024;
}
