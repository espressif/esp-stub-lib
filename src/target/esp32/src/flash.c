/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <target/flash.h>
#include <private/rom_flash.h>

extern esp_rom_spiflash_chip_t g_rom_flashchip;
extern uint8_t g_rom_spiflash_dummy_len_plus[];

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
    // TODO: Implement
    return 0;
}

stub_lib_err_t stub_target_flash_read_buff(uint32_t addr, void *buffer, uint32_t size)
{
    (void)addr;
    (void)buffer;
    (void)size;
    // TODO: Implement
    return STUB_LIB_FAIL;
}
