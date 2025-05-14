/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <target/flash.h>

#include <stdbool.h>
#include <stddef.h>

#include <log.h>
#include <private/rom.h>
#include <private/soc_utils.h>

#include "soc/reg_base.h"
#include "soc/spi_reg.h"

#define SPI_FLASH_CMD_RDID 0x9F
#define ESP_ROM_SPIFLASH_BUSY_FLAG BIT(0)

#define GPIO_STRAP_REG (DR_REG_GPIO_BASE + 0x0038)
#define PERIPHS_SPI_FLASH_CMD SPI_CMD_REG(1)
#define PERIPHS_SPI_FLASH_STATUS SPI_RD_STATUS_REG(1)
#define PERIPHS_SPI_FLASH_USRREG SPI_USER_REG(1)
#define PERIPHS_SPI_FLASH_USRREG2 SPI_USER2_REG(1)
#define PERIPHS_SPI_FLASH_C0 SPI_W0_REG(1)
#define PERIPHS_SPI_MOSI_DLEN_REG SPI_MOSI_DLEN_REG(1)
#define PERIPHS_SPI_MISO_DLEN_REG SPI_MISO_DLEN_REG(1)

/* ROM */

extern uint32_t ets_efuse_get_spiconfig(void);
extern void esp_rom_spiflash_attach(uint32_t ishspi, bool legacy);
extern esp_rom_spiflash_chip_t g_rom_flashchip;
extern uint8_t g_rom_spiflash_dummy_len_plus[];

// TODO: common code with flash_spi_wait_ready
static uint32_t flash_exec_usr_cmd(uint32_t cmd)
{
    uint32_t status_value = ESP_ROM_SPIFLASH_BUSY_FLAG;

    while (ESP_ROM_SPIFLASH_BUSY_FLAG == (status_value & ESP_ROM_SPIFLASH_BUSY_FLAG)) {
        WRITE_PERI_REG(PERIPHS_SPI_FLASH_STATUS, 0); /* clear register */
        WRITE_PERI_REG(PERIPHS_SPI_FLASH_CMD, SPI_USR | cmd); //TODO cmd is bit(0), this is a useless bit
        while (READ_PERI_REG(PERIPHS_SPI_FLASH_CMD) != 0)
            ;
        status_value = READ_PERI_REG(PERIPHS_SPI_FLASH_STATUS) & g_rom_flashchip.status_mask;
    }

    return status_value;
}

static void flash_spi_wait_ready(void)
{
    uint32_t status_value = ESP_ROM_SPIFLASH_BUSY_FLAG;

    while (ESP_ROM_SPIFLASH_BUSY_FLAG == (status_value & ESP_ROM_SPIFLASH_BUSY_FLAG)) {
        WRITE_PERI_REG(PERIPHS_SPI_FLASH_STATUS, 0); /* clear register */
        WRITE_PERI_REG(PERIPHS_SPI_FLASH_CMD, SPI_FLASH_RDSR);
        while (READ_PERI_REG(PERIPHS_SPI_FLASH_CMD) != 0)
            ;
        status_value = READ_PERI_REG(PERIPHS_SPI_FLASH_STATUS) & g_rom_flashchip.status_mask;
    }
}

static uint32_t flash_spi_cmd_run(uint32_t cmd,
                                  uint8_t data_bits[],
                                  uint32_t data_bits_num,
                                  uint32_t read_bits_num)
{
    uint32_t old_spi_usr = READ_PERI_REG(PERIPHS_SPI_FLASH_USRREG);
    uint32_t old_spi_usr2 = READ_PERI_REG(PERIPHS_SPI_FLASH_USRREG2);
    uint32_t flags = SPI_USR_COMMAND;

    flash_spi_wait_ready();

    if (read_bits_num > 0) {
        flags |= SPI_USR_MISO;
        WRITE_PERI_REG(PERIPHS_SPI_MISO_DLEN_REG, read_bits_num - 1);
    }
    if (data_bits_num > 0) {
        flags |= SPI_USR_MOSI;
        WRITE_PERI_REG(PERIPHS_SPI_MOSI_DLEN_REG, data_bits_num - 1);
    }

    WRITE_PERI_REG(PERIPHS_SPI_FLASH_USRREG, flags);
    WRITE_PERI_REG(PERIPHS_SPI_FLASH_USRREG2, (7 << SPI_USR_COMMAND_BITLEN_S) | cmd);
    if (data_bits_num == 0) {
        WRITE_PERI_REG(PERIPHS_SPI_FLASH_C0, 0);
    } else {
        for (uint32_t i = 0; i <= data_bits_num / 32; i += 32) {
            WRITE_PERI_REG(PERIPHS_SPI_FLASH_C0 + i / 8, *((uint32_t *)&data_bits[i / 8]));
        }
    }
    flash_exec_usr_cmd(0);
    uint32_t status = READ_PERI_REG(PERIPHS_SPI_FLASH_C0);
    /* restore some SPI controller registers */
    WRITE_PERI_REG(PERIPHS_SPI_FLASH_USRREG, old_spi_usr);
    WRITE_PERI_REG(PERIPHS_SPI_FLASH_USRREG2, old_spi_usr2);

    return status;
}

static inline uint32_t device_id_from_spi_rdid()
{

    uint32_t rdid = flash_spi_cmd_run(SPI_FLASH_CMD_RDID, NULL, 0, 24);
    return ((rdid & 0xff) << 16) | (rdid & 0xff00) | ((rdid & 0xff0000) >> 16);
}

void stub_target_flash_init(void *state)
{
    (void)state;
    STUB_LOG_TRACE();
    uint32_t spiconfig = ets_efuse_get_spiconfig();

    uint32_t strapping = REG_READ(GPIO_STRAP_REG);
    /*  If GPIO1 (U0TXD) is pulled low and flash pin configuration is not set in efuse, assume
     * HSPI flash mode (same as normal boot) */
    if (spiconfig == 0 && (strapping & 0x1c) == 0x08) {
        spiconfig = 1;    /* HSPI flash mode */
    }

    /* 0x1c bits are:
            0x04 bit - print control
            0x08 bit - flash boot (SPI boot)
            0x10 bit - HSPI boot?

    */

    // TODO: do we need legacy arg = !ETS_IS_FAST_FLASH_BOOT() ?
    esp_rom_spiflash_attach(spiconfig, false);
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
    uint32_t id = device_id_from_spi_rdid();
    STUB_LOG_TRACEF("Device ID: 0x%x (from SPI RDID)\n", id);
    return id;
}

void stub_target_flash_unlock(void)
{
    // TODO: Add the patched version to rom_common
}

const esp_rom_spiflash_chip_t *stub_target_flash_get_rom_flashchip(void)
{
    return &g_rom_flashchip;
}
