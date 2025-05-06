/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>

struct esp_rom_spiflash_chip;

void stub_target_flash_init(void *state);
void stub_target_flash_deinit(const void *state);
uint32_t stub_target_flash_device_id(void);
void stub_target_flash_unlock(void);
const struct esp_rom_spiflash_chip *stub_target_flash_get_rom_flashchip(void);
