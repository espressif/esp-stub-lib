/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <target/flash.h>

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

uint32_t stub_target_flash_device_id(void)
{
    // TODO: Implement
    return 0;
}

void stub_target_flash_unlock(void)
{
    // TODO: Implement
}

const struct esp_rom_spiflash_chip *stub_target_flash_get_rom_flashchip(void)
{
    // TODO: Implement
    return 0;
}
