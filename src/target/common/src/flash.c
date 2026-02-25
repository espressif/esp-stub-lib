/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stddef.h>
#include <stdint.h>

#include <bit_utils.h>
#include <log.h>

#include <esp-stub-lib/err.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/flash.h>

#include <private/rom_flash.h>
#include <private/rom_flash_config.h>

#include <soc/io_mux_reg.h>
#include <soc/spi_mem_compat.h>

extern int esp_rom_spiflash_erase_chip(void);
extern void esp_rom_spiflash_attach(uint32_t ishspi, bool legacy);
extern int esp_rom_spiflash_write(uint32_t flash_addr, const void *data, uint32_t size);
extern int esp_rom_spiflash_write_encrypted(uint32_t flash_addr, const void *data, uint32_t size);
extern int esp_rom_spiflash_erase_sector(uint32_t addr);
extern int esp_rom_spiflash_erase_block(uint32_t addr);
extern int esp_rom_spiflash_erase_area(uint32_t addr, uint32_t size);
extern int esp_rom_spiflash_write_enable(esp_rom_spiflash_chip_t *flash_chip);
extern void esp_rom_spiflash_write_encrypted_enable(void);
extern void esp_rom_spiflash_write_encrypted_disable(void);
extern int esp_rom_spiflash_unlock(void);

extern void esp_rom_gpio_pad_select_gpio(uint32_t gpio_num);

void __attribute__((weak)) stub_target_reset_default_spi_pins(void)
{
#ifdef SPI_CLK_GPIO_NUM
    esp_rom_gpio_pad_select_gpio(SPI_CLK_GPIO_NUM);
    esp_rom_gpio_pad_select_gpio(SPI_Q_GPIO_NUM);
    esp_rom_gpio_pad_select_gpio(SPI_D_GPIO_NUM);
    esp_rom_gpio_pad_select_gpio(SPI_CS0_GPIO_NUM);
#endif
}

int stub_target_flash_update_config(uint32_t flash_id,
                                    uint32_t flash_size,
                                    uint32_t block_size,
                                    uint32_t sector_size,
                                    uint32_t page_size,
                                    uint32_t status_mask)
{
    int res = esp_rom_spiflash_config_param(flash_id, flash_size, block_size, sector_size, page_size, status_mask);
    if (res != ESP_ROM_SPIFLASH_RESULT_OK) {
        STUB_LOGE("Failed to update flash config: %d\n", res);
        return STUB_LIB_FAIL;
    }
    return STUB_LIB_OK;
}

uint32_t stub_target_flash_id_to_flash_size(uint32_t flash_id)
{
    const uint8_t id = flash_id & 0xff;
    switch (id) {
    case 0x12: // 256 KB
    case 0x13:
    case 0x14: // 1 MB
    case 0x15:
    case 0x16:
    case 0x17:
    case 0x18:
    case 0x19: // 32 MB
    case 0x1A: // 64 MB
    case 0x1B: // 128 MB
    case 0x1C: // 256 MB
        return 1u << id;
    case 0x39:
        return MIB(32);
    }

    STUB_LOGE("Unknown flash_id: 0x%x\n", flash_id);
    return 0;
}

uint32_t __attribute__((weak)) stub_target_get_max_supported_flash_size(void)
{
    /* Default: chips without 4-byte addressing support (16MB max) */
    return 16 * 1024 * 1024;
}

uint32_t __attribute__((weak)) stub_target_flash_get_spiconfig_efuse(void)
{
    return 0;
}

bool __attribute__((weak)) stub_target_flash_state_save(void **state)
{
    if (state == NULL) {
        return true;
    }

    /* Until all targets have state saving implemented, we need to return true */
    return true;
}

void __attribute__((weak)) stub_target_flash_attach(uint32_t ishspi, bool legacy)
{
    esp_rom_spiflash_attach(ishspi, legacy);
}

void __attribute__((weak)) stub_target_flash_init(void **state)
{
    bool attach_required = stub_target_flash_state_save(state);

    STUB_LOG_TRACEF("attach_required: %d\n", attach_required);

    if (attach_required) {
        uint32_t spiconfig = stub_target_flash_get_spiconfig_efuse();
        stub_target_flash_attach(spiconfig, 0);
    }
}

esp_rom_spiflash_chip_t *__attribute__((weak)) stub_target_flash_get_config(void)
{
    extern esp_rom_spiflash_legacy_data_t *rom_spiflash_legacy_data;
    return &rom_spiflash_legacy_data->chip;
}

uint32_t __attribute__((weak)) stub_target_flash_get_flash_id(void)
{
    esp_rom_spi_flash_update_id();
    return stub_target_flash_get_config()->flash_id;
}

void __attribute__((weak)) stub_target_flash_state_restore(const void *state)
{
    (void)state;
}

void __attribute__((weak)) stub_target_flash_write_enable(void)
{
    esp_rom_spiflash_chip_t *chip = stub_target_flash_get_config();
    esp_rom_spiflash_write_enable(chip);
}

int __attribute__((weak)) stub_target_flash_write_buff(uint32_t addr, const void *buffer, uint32_t size, bool encrypt)
{
    int res;
    if (encrypt) {
        res = esp_rom_spiflash_write_encrypted(addr, buffer, size);
    } else {
        res = esp_rom_spiflash_write(addr, buffer, size);
    }
    return res == ESP_ROM_SPIFLASH_RESULT_OK ? STUB_LIB_OK : STUB_LIB_ERR_FLASH_WRITE;
}

int __attribute__((weak)) stub_target_flash_read_buff(uint32_t addr, void *buffer, uint32_t size)
{
    int res = esp_rom_spiflash_read(addr, buffer, (int32_t)size);
    return res == ESP_ROM_SPIFLASH_RESULT_OK ? STUB_LIB_OK : STUB_LIB_ERR_FLASH_READ;
}

int __attribute__((weak)) stub_target_flash_erase_chip(void)
{
    if (esp_rom_spiflash_erase_chip() == ESP_ROM_SPIFLASH_RESULT_OK) {
        return STUB_LIB_OK;
    }
    return STUB_LIB_FAIL;
}

bool __attribute__((weak)) stub_target_flash_is_busy(void)
{
    stub_target_spi_wait_ready();

    REG_WRITE(SPI_MEM_RD_STATUS_REG(FLASH_SPI_NUM), 0);
    REG_WRITE(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_FLASH_RDSR);
    while (REG_READ(SPI_MEM_CMD_REG(FLASH_SPI_NUM)) != 0) {
        /* busy wait */
    }
    uint32_t status_value = REG_READ(SPI_MEM_RD_STATUS_REG(FLASH_SPI_NUM));

    return (status_value & BIT(0)) != 0;
}

void __attribute__((weak)) stub_target_flash_erase_sector_start(uint32_t addr)
{
    stub_target_flash_write_enable();
    stub_target_spi_wait_ready();

    REG_WRITE(SPI_MEM_ADDR_REG(FLASH_SPI_NUM), addr & 0xffffff);
    REG_WRITE(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_FLASH_SE);
    while (REG_READ(SPI_MEM_CMD_REG(FLASH_SPI_NUM)) != 0) {
        /* busy wait */
    }

    STUB_LOGV("Started sector erase at 0x%x\n", addr);
}

void __attribute__((weak)) stub_target_flash_erase_block_start(uint32_t addr)
{
    stub_target_flash_write_enable();
    stub_target_spi_wait_ready();

    REG_WRITE(SPI_MEM_ADDR_REG(FLASH_SPI_NUM), addr & 0xffffff);
    REG_WRITE(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_FLASH_BE);
    while (REG_READ(SPI_MEM_CMD_REG(FLASH_SPI_NUM)) != 0) {
        /* busy wait */
    }

    STUB_LOGV("Started block erase at 0x%x\n", addr);
}

void __attribute__((weak)) stub_target_opiflash_exec_cmd(const opiflash_cmd_params_t *params)
{
    /* Empty implementation for targets without large flash support */
    (void)params;
}

void __attribute__((weak)) stub_target_flash_write_encrypted_enable(void)
{
    esp_rom_spiflash_write_encrypted_enable();
}

void __attribute__((weak)) stub_target_flash_write_encrypted_disable(void)
{
    esp_rom_spiflash_write_encrypted_disable();
}

int __attribute__((weak)) stub_target_flash_unlock(void)
{
    if (esp_rom_spiflash_unlock() == ESP_ROM_SPIFLASH_RESULT_OK) {
        return STUB_LIB_OK;
    }
    return STUB_LIB_FAIL;
}
