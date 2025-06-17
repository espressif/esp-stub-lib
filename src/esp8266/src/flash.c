/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <log.h>
#include <target/flash.h>
#include <target/impl/flash_get_config_from_rom_old.h>

void stub_target_flash_init(void)
{
    STUB_LOG_TRACE();
    // TODO: Implement
}

uint32_t stub_target_flash_get_flash_id(void)
{
    STUB_LOG_TRACE();
    // TODO: Implement
    return 0;
}

const esp_rom_spiflash_chip_t * stub_target_flash_get_config(void)
{
    return flash_impl_get_config_from_rom_old();
}
