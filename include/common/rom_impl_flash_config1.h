/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <common/target_rom.h>

/* ROM's flash config data for some old chips */
extern esp_rom_spiflash_chip_t g_rom_flashchip;

const esp_rom_spiflash_chip_t *stub_target_rom_get_flash_config(void)
{
    return &g_rom_flashchip;
}
