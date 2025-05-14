/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <target/flash.h>

#include <stdbool.h>

#include <log.h>
#include <private/rom.h>
#include <private/soc_utils.h>

#include "soc/spi_mem_reg.h"

/* ROM */

typedef struct {
    esp_rom_spiflash_chip_t chip;
    uint8_t dummy_len_plus[3];
    uint8_t sig_matrix;
} esp_rom_spiflash_legacy_data_t;

extern void esp_rom_spi_flash_update_id(void);
extern void spi_flash_attach(uint32_t ishspi, bool legacy);
extern int esp_rom_spiflash_unlock(void);
extern esp_rom_spiflash_legacy_data_t *rom_spiflash_legacy_data;

#define g_rom_flashchip (rom_spiflash_legacy_data->chip)
#define g_rom_spiflash_dummy_len_plus (rom_spiflash_legacy_data->dummy_len_plus)

__attribute__((unused)) static inline uint32_t device_id_from_spi_rdid()
{
    WRITE_PERI_REG(SPI_MEM_W0_REG(1), 0);
    WRITE_PERI_REG(SPI_MEM_CMD_REG(1), SPI_MEM_FLASH_RDID);
    while (READ_PERI_REG(SPI_MEM_CMD_REG(1)) != 0)
        ;
    uint32_t rdid = READ_PERI_REG(SPI_MEM_W0_REG(1)) & 0xffffff;
    return ((rdid & 0xff) << 16) | (rdid & 0xff00) | ((rdid & 0xff0000) >> 16);;
}

__attribute__((unused)) static inline uint32_t device_id_from_rom()
{
    // Sets the correct g_rom_flashchip.device_id from SPI_MEM_FLASH_RDID in ROM code
    esp_rom_spi_flash_update_id();
    return g_rom_flashchip.device_id;
}

void stub_target_flash_init(void *state)
{
    (void)state;
    STUB_LOG_TRACE();
    const uint32_t NO_CONFIG_SPI = 0;
    spi_flash_attach(NO_CONFIG_SPI, false);
}

void stub_target_flash_deinit(const void *state)
{
    (void)state;
    // TODO: Implement
}

uint32_t stub_target_flash_device_id(void)
{
    // TODO: remove dev tracing
    STUB_LOG_TRACEF("Uninit g_rom_flashchip.device_id: 0x%x\n", g_rom_flashchip.device_id);

    // TODO: it's just for development. remove this option then
    uint32_t rdid = device_id_from_spi_rdid();
    (void)rdid;
    STUB_LOG_TRACEF("Device ID: 0x%x (from SPI RDID)\n", rdid);

    uint32_t id = device_id_from_rom();
    STUB_LOG_TRACEF("Device ID: 0x%x (from ROM code)\n", g_rom_flashchip.device_id);
    return id;
}

void stub_target_flash_unlock(void)
{
    STUB_LOG_TRACE();
    esp_rom_spiflash_unlock();
}

const esp_rom_spiflash_chip_t *stub_target_flash_get_rom_flashchip(void)
{
    return &g_rom_flashchip;
}
