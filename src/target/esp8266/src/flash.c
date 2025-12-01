/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <stdbool.h>
#include <log.h>
#include <err.h>
#include <target/flash.h>
#include <private/rom_flash.h>

extern esp_rom_spiflash_result_t esp_rom_spiflash_write(uint32_t flash_addr, const void *data, uint32_t size);

int stub_target_flash_write_buff(uint32_t addr, const void *buffer, uint32_t size, bool encrypt)
{
    if (addr & 3 || size & 3) {
        STUB_LOGE("Unaligned write: 0x%x, %u\n", addr, size);
        return STUB_LIB_ERR_FLASH_WRITE_UNALIGNED;
    }
    if (encrypt) {
        return STUB_LIB_ERR_NOT_SUPPORTED;
    }
    esp_rom_spiflash_result_t res = esp_rom_spiflash_write(addr, buffer, size);
    STUB_LOG_TRACEF("(0x%x, 0x%x, %u, %d) results: %d\n", addr, (uint32_t)buffer, size, encrypt, res);
    return res == ESP_ROM_SPIFLASH_RESULT_OK ? STUB_LIB_OK : STUB_LIB_FAIL;
}
