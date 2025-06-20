/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>

#include <log.h>

#include <target/flash.h>
#include <private/rom_flash.h>

void stub_target_flash_init(void)
{
    STUB_LOG_TRACE();
    esp_rom_spiflash_attach(0, false);
}

uint32_t stub_target_flash_get_flash_id(void)
{
    // TODO: remove dev tracing
    STUB_LOG_TRACEF("Uninit ROM's flash_id: 0x%x\n", stub_target_flash_get_config()->flash_id);

    esp_rom_spi_flash_update_id();
    STUB_LOG_TRACEF("Flash ID: 0x%x (from ROM code)\n", stub_target_flash_get_config()->flash_id);
    return stub_target_flash_get_config()->flash_id;
}
