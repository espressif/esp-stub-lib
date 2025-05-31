/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>

#include <log.h>
#include <target/flash.h>
#include <private/soc_utils.h>

#include <target_spec/periph_defs.h>

/**
  * @brief Sets the correct flash_id in the flash config from SPI_MEM_FLASH_RDID in ROM code
  *
  */
extern void esp_rom_spi_flash_update_id(void);

inline static
uint32_t flash_impl_get_id_from_rdid_reg(void)
{
    WRITE_PERI_REG(PERIPHS_SPI_FLASH_C0, 0);
    WRITE_PERI_REG(PERIPHS_SPI_FLASH_CMD, PERIPHS_SPI_FLASH_BITS_RDID);
    while (READ_PERI_REG(PERIPHS_SPI_FLASH_CMD) != 0)
        ;
    uint32_t rdid = READ_PERI_REG(PERIPHS_SPI_FLASH_C0) & 0xffffff;
    return ((rdid & 0xff) << 16) | (rdid & 0xff00) | ((rdid & 0xff0000) >> 16);;
}

inline static
uint32_t flash_impl_get_id_from_config(void)
{

    esp_rom_spi_flash_update_id();
    return stub_target_flash_get_config()->flash_id;
}

inline static
uint32_t flash_impl_get_id_from_rom(void)
{
    // TODO: remove dev tracing
    STUB_LOG_TRACEF("Uninit ROM's flash_id: 0x%x\n", stub_target_flash_get_config()->flash_id);

    // TODO: it's just for development. remove this option then
    uint32_t rdid = flash_impl_get_id_from_rdid_reg();
    (void)rdid;
    STUB_LOG_TRACEF("Flash ID: 0x%x (from SPI RDID)\n", rdid);

    uint32_t id = flash_impl_get_id_from_config();
    STUB_LOG_TRACEF("Flash ID: 0x%x (from ROM code)\n", stub_target_flash_get_config()->flash_id);
    return id;
}
