/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <log.h>

/**
  * @brief SPI Flash init
  *
  * @param spiconfig: 0 - auto from efuse; a special value - for pins
  *
  * @param legacy: Deprecated compatibility API value, must be false
  *
  */
extern void esp_rom_spiflash_attach(uint32_t spiconfig, bool legacy);

inline static
void flash_impl_init_auto_spiconfig(void)
{
    STUB_LOG_TRACE();
    // Do not call ets_efuse_get_spiconfig here because
    // it is called inside of esp_rom_spiflash_attach() when spiconfig=0
    esp_rom_spiflash_attach(0, false);
}
