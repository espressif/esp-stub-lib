/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <target/flash.h>
#include <private/rom_flash.h>
#include <private/rom_flash_config.h>

void stub_target_flash_init(void *state)
{
    (void)state;
    // TODO: Implement
}

void stub_target_flash_deinit(const void *state)
{
    (void)state;
    // TODO: Implement
}

const struct esp_rom_spiflash_chip *stub_target_flash_get_config(void)
{
    return &g_rom_flashchip;
}

uint32_t stub_target_flash_get_flash_id(void)
{
    esp_rom_spi_flash_update_id();
    return stub_target_flash_get_config()->flash_id;
}
