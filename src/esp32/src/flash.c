/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stddef.h>

#include <log.h>
#include <bit_utils.h>
#include <private/soc_utils.h>
#include <target/flash.h>
#include <target/impl/flash_get_config_from_rom_old.h>

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

/* XXX0_10XX */
#define IS_HSPI_BOOT(strap_val) (((strap_val)&0x1c)==0x08)

/* ROM */

/**
  * @brief  Read spi flash pin configuration from Efuse
  *
  * @return
  * - 0 for default SPI pins.
  * - 1 for default HSPI pins.
  * - Other values define a custom pin configuration mask. Pins are encoded as per the EFUSE_SPICONFIG_RET_SPICLK,
  *   EFUSE_SPICONFIG_RET_SPIQ, EFUSE_SPICONFIG_RET_SPID, EFUSE_SPICONFIG_RET_SPICS0, EFUSE_SPICONFIG_RET_SPIHD macros.
  *   WP pin (for quad I/O modes) is not saved in efuse and not returned by this function.
  */
extern uint32_t ets_efuse_get_spiconfig(void);
/**
  * @brief SPI Flash init, clock divisor is 4, use 1 line Slow read mode.
  *    Please do not call this function in SDK.
  *
  * @param  uint32_t ishspi: 0 for spi, 1 for hspi, flash pad decided by strapping
  *              else, bit[5:0] spiclk, bit[11:6] spiq, bit[17:12] spid, bit[23:18] spics0, bit[29:24] spihd
  *
  * @param  uint8_t legacy: always keeping false.
  *
  * @return None
  */
extern void esp_rom_spiflash_attach(uint32_t ishspi, bool legacy);

// TODO: avoid common code and update execute_command()
static uint32_t flash_exec_usr_cmd(uint32_t cmd)
{
    uint32_t status_value = ESP_ROM_SPIFLASH_BUSY_FLAG;

    while (ESP_ROM_SPIFLASH_BUSY_FLAG == (status_value & ESP_ROM_SPIFLASH_BUSY_FLAG)) {
        WRITE_PERI_REG(PERIPHS_SPI_FLASH_STATUS, 0); /* clear register */
        WRITE_PERI_REG(PERIPHS_SPI_FLASH_CMD, SPI_USR | cmd); //TODO cmd is bit(0), this is a useless bit
        while (READ_PERI_REG(PERIPHS_SPI_FLASH_CMD) != 0)
            ;
        status_value = READ_PERI_REG(PERIPHS_SPI_FLASH_STATUS) & stub_target_flash_get_config()->status_mask;
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
        status_value = READ_PERI_REG(PERIPHS_SPI_FLASH_STATUS) & stub_target_flash_get_config()->status_mask;
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

static inline uint32_t get_id_from_rdid_cmd(void)
{
    uint32_t rdid = flash_spi_cmd_run(SPI_FLASH_CMD_RDID, NULL, 0, 24);
    return ((rdid & 0xff) << 16) | (rdid & 0xff00) | ((rdid & 0xff0000) >> 16);
}

void stub_target_flash_init(void)
{
    STUB_LOG_TRACE();
    uint32_t ishspi = 0;
    uint32_t spiconfig = ets_efuse_get_spiconfig();
    const uint32_t strapping = REG_READ(GPIO_STRAP_REG);
    if (IS_HSPI_BOOT(strapping)) {
        ishspi = 1;
    }
    if (spiconfig) {
        ishspi = spiconfig;
    }
    esp_rom_spiflash_attach(ishspi, false);
}

uint32_t stub_target_flash_get_flash_id(void)
{
    // TODO: remove dev tracing
    STUB_LOG_TRACEF("Uninit ROM's flash_id: 0x%x\n", stub_target_flash_get_config()->flash_id);
    uint32_t id = get_id_from_rdid_cmd();
    STUB_LOG_TRACEF("Flash ID: 0x%x (from SPI RDID)\n", id);
    return id;
}

const esp_rom_spiflash_chip_t * stub_target_flash_get_config(void)
{
    return flash_impl_get_config_from_rom_old();
}
