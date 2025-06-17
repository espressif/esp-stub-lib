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
  * @brief SPI Flash init. CONFIG SPI is unsupported
  *
  * @param spiconfig: Deprecated compatibility API value, must be 0
  *
  * @param legacy: Deprecated compatibility API value, must be false
  *
  */
extern void esp_rom_spiflash_attach(uint32_t spiconfig, bool legacy);

inline static
void flash_impl_init_no_spiconfig(void)
{
    STUB_LOG_TRACE();
    esp_rom_spiflash_attach(0, false);
}
