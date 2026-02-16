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

/* ECO version from ROM - used to route to correct ROM functions */
extern uint32_t _rom_eco_version;
extern void esp_rom_spiflash_attach(uint32_t ishspi, bool legacy);
extern void esp_rom_opiflash_exec_cmd_eco1(int spi_num,
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

extern void esp_rom_opiflash_exec_cmd_eco5(int spi_num,
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

extern void esp_rom_opiflash_exec_cmd_eco6(int spi_num,
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
    if (_rom_eco_version == 2) {
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
    } else if (_rom_eco_version == 5) {
        esp_rom_opiflash_exec_cmd_eco5(params->spi_num,
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
    } else if (_rom_eco_version < 5) {
        esp_rom_opiflash_exec_cmd_eco1(params->spi_num,
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
        esp_rom_opiflash_exec_cmd_eco6(params->spi_num,
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

void stub_target_flash_attach(uint32_t ishspi, bool legacy)
{
    esp_rom_spiflash_attach(ishspi, legacy);
}

void stub_target_reset_default_spi_pins(void)
{
    /* ESP32-P4 uses dedicated pins for SPI flash. */
}

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
    while (REG_READ(SPI1_MEM_C_CMD_REG) != 0) {
    }
    uint32_t status_value = REG_READ(SPI1_MEM_C_RD_STATUS_REG);

    return (status_value & STATUS_BUSY_BIT) != 0;
}

void stub_target_flash_erase_sector_start(uint32_t addr)
{
    stub_target_flash_write_enable();
    spi_wait_ready();

    REG_WRITE(SPI1_MEM_C_ADDR_REG, addr & 0xffffff);
    REG_WRITE(SPI1_MEM_C_CMD_REG, SPI1_MEM_C_FLASH_SE);
    while (REG_READ(SPI1_MEM_C_CMD_REG) != 0) {
    }

    STUB_LOGV("Started sector erase at 0x%x\n", addr);
}

void stub_target_flash_erase_block_start(uint32_t addr)
{
    stub_target_flash_write_enable();
    spi_wait_ready();

    REG_WRITE(SPI1_MEM_C_ADDR_REG, addr & 0xffffff);
    REG_WRITE(SPI1_MEM_C_CMD_REG, SPI1_MEM_C_FLASH_BE);
    while (REG_READ(SPI1_MEM_C_CMD_REG) != 0) {
    }

    STUB_LOGV("Started block erase at 0x%x\n", addr);
}

uint32_t stub_target_get_max_supported_flash_size(void)
{
    /* ESP32-P4 supports up to 64MB with 4-byte addressing */
    return 64 * 1024 * 1024;
}
