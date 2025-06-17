/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <log.h>
#include <target/flash.h>
#include <target/impl/flash_init_auto_spiconfig.h>
#include <target/impl/flash_get_id_from_rom.h>
#include <target/impl/flash_get_config_from_rom_old.h>

void stub_target_flash_init(void)
{
    STUB_LOG_TRACE();
    flash_impl_init_auto_spiconfig();
}

uint32_t stub_target_flash_get_flash_id(void)
{
    STUB_LOG_TRACE();
    return flash_impl_get_id_from_rom();
}

const esp_rom_spiflash_chip_t * stub_target_flash_get_config(void)
{
    return flash_impl_get_config_from_rom_old();
}
