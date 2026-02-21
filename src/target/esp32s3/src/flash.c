/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <stdbool.h>
#include <log.h>
#include <err.h>
#include <target/flash.h>
#include <private/rom_flash.h>
#include <private/rom_flash_config.h>
#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/soc_utils.h>
#include <soc/spi_mem_reg.h>
#include <soc/io_mux_reg.h>

#define SPI_INTERNAL    0
#define FLASH_SPI_NUM   1
#define STATUS_BUSY_BIT BIT(0)

extern uint32_t ets_efuse_get_spiconfig(void);
extern esp_rom_spiflash_legacy_funcs_t *rom_spiflash_legacy_funcs;
extern uint32_t esp_rom_efuse_get_flash_gpio_info(void);
extern void esp_rom_spiflash_attach(uint32_t spiconfig, bool legacy);
extern void esp_rom_opiflash_exec_cmd(int spi_num,
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

void stub_target_flash_attach(uint32_t ishspi, bool legacy)
{
    esp_rom_spiflash_attach(ishspi, legacy);
}

uint32_t stub_target_flash_get_spiconfig_efuse(void)
{
    return esp_rom_efuse_get_flash_gpio_info();
}

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
    uint32_t spiconfig = stub_target_flash_get_spiconfig_efuse();
    esp_rom_spiflash_attach(spiconfig, 0);
    if (ets_efuse_flash_octal_mode()) {
        STUB_LOGD("octal mode is on\n");
        stub_target_flash_init_funcs();
    }
}

void stub_target_opiflash_exec_cmd(const opiflash_cmd_params_t *params)
{
    esp_rom_opiflash_exec_cmd(params->spi_num,
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

static void spi_wait_ready(void)
{
    while (REG_GET_FIELD(SPI_MEM_FSM_REG(FLASH_SPI_NUM), SPI_MEM_ST)) {
        /* busy wait */
    }
    /* There is no HW arbiter on SPI_INTERNAL, so we need to wait for it to be ready */
    while ((REG_READ(SPI_MEM_FSM_REG(SPI_INTERNAL)) & SPI_MEM_ST)) {
        /* busy wait */
    }
}

bool stub_target_flash_is_busy(void)
{
    spi_wait_ready();

    REG_WRITE(SPI_MEM_RD_STATUS_REG(FLASH_SPI_NUM), 0);
    REG_WRITE(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_FLASH_RDSR);
    while (REG_READ(SPI_MEM_CMD_REG(FLASH_SPI_NUM)) != 0) {
    }
    uint32_t status_value = REG_READ(SPI_MEM_RD_STATUS_REG(FLASH_SPI_NUM));

    return (status_value & STATUS_BUSY_BIT) != 0;
}

void stub_target_flash_erase_sector_start(uint32_t addr)
{
    stub_target_flash_write_enable();
    spi_wait_ready();

    REG_WRITE(SPI_MEM_ADDR_REG(FLASH_SPI_NUM), addr & 0xffffff);
    REG_WRITE(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_FLASH_SE);
    while (REG_READ(SPI_MEM_CMD_REG(FLASH_SPI_NUM)) != 0) {
    }

    STUB_LOGV("Started sector erase at 0x%x\n", addr);
}

void stub_target_flash_erase_block_start(uint32_t addr)
{
    stub_target_flash_write_enable();
    spi_wait_ready();

    REG_WRITE(SPI_MEM_ADDR_REG(FLASH_SPI_NUM), addr & 0xffffff);
    REG_WRITE(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_FLASH_BE);
    while (REG_READ(SPI_MEM_CMD_REG(FLASH_SPI_NUM)) != 0) {
    }

    STUB_LOGV("Started block erase at 0x%x\n", addr);
}

uint32_t stub_target_get_max_supported_flash_size(void)
{
    /* ESP32-S3 supports up to 1GB with 4-byte addressing */
    return 1024 * 1024 * 1024;
}
