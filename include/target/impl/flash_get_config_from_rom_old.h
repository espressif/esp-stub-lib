/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <target/flash.h>

/* ROM's flash config data for some old chips */
extern esp_rom_spiflash_chip_t g_rom_flashchip;

inline static
const esp_rom_spiflash_chip_t *flash_impl_get_config_from_rom_old(void)
{
    return &g_rom_flashchip;
}
