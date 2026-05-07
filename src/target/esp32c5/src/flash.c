/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/log.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>
#include <target/flash.h>

#include <private/rom_flash_config.h>

#include <soc/spi_mem_compat.h>

extern int esp_rom_spiflash_config_readmode(int mode, bool legacy);
extern void SelectSpiFunction(uint32_t ishspi);
extern void spi_common_set_flash_cs_timing(void);
extern void spi_cache_mode_switch(uint32_t modebit);
extern void esp_rom_spiflash_attach(uint32_t ishspi, bool legacy);

extern esp_rom_spiflash_legacy_data_t *rom_spiflash_legacy_data;

/* ECO version from ROM - used to route to correct ROM functions */
extern uint32_t _rom_eco_version;
/* ECO-specific ROM function declarations */
extern void esp_rom_opiflash_exec_cmd_eco2(int spi_num,
                                           spi_flash_mode_t mode,
                                           uint32_t cmd,
                                           int cmd_bit_len,
                                           uint32_t addr,
                                           int addr_bit_len,
                                           int dummy_bits,
                                           const uint8_t *mosi_data,
                                           int mosi_bit_len,
                                           uint8_t *miso_data,
                                           int miso_bit_len,
                                           uint32_t cs_mask,
                                           bool is_write_erase_operation);

extern void esp_rom_opiflash_exec_cmd_eco3(int spi_num,
                                           spi_flash_mode_t mode,
                                           uint32_t cmd,
                                           int cmd_bit_len,
                                           uint32_t addr,
                                           int addr_bit_len,
                                           int dummy_bits,
                                           const uint8_t *mosi_data,
                                           int mosi_bit_len,
                                           uint8_t *miso_data,
                                           int miso_bit_len,
                                           uint32_t cs_mask,
                                           bool is_write_erase_operation);

void stub_target_opiflash_exec_cmd(const opiflash_cmd_params_t *params)
{
    if (_rom_eco_version >= 3) {
        esp_rom_opiflash_exec_cmd_eco3(params->spi_num,
                                       params->mode,
                                       params->cmd,
                                       params->cmd_bit_len,
                                       params->addr,
                                       params->addr_bit_len,
                                       params->dummy_bits,
                                       params->mosi_data,
                                       params->mosi_bit_len,
                                       params->miso_data,
                                       params->miso_bit_len,
                                       params->cs_mask,
                                       params->is_write_erase_operation);
    } else {
        esp_rom_opiflash_exec_cmd_eco2(params->spi_num,
                                       params->mode,
                                       params->cmd,
                                       params->cmd_bit_len,
                                       params->addr,
                                       params->addr_bit_len,
                                       params->dummy_bits,
                                       params->mosi_data,
                                       params->mosi_bit_len,
                                       params->miso_data,
                                       params->miso_bit_len,
                                       params->cs_mask,
                                       params->is_write_erase_operation);
    }
}

void stub_target_spi_wait_ready(void)
{
    while (REG_GET_FIELD(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_MST_ST)) {
        /* busy wait */
    }
}

uint32_t stub_target_get_max_supported_flash_size(void)
{
    /* ESP32-C5 supports up to 32MB with 4-byte addressing */
    return MIB(32);
}

uint32_t __attribute__((weak)) stub_target_flash_get_flash_id(void)
{
    esp_rom_spi_flash_update_id();
    return stub_target_flash_get_config()->flash_id;
}

bool stub_target_flash_needs_attach(void)
{
    return !stub_target_cache_is_enabled();
}

void stub_target_flash_init(void *state, stub_lib_flash_attach_policy_t attach_policy)
{
    (void)state;

    if (attach_policy == STUB_LIB_FLASH_ATTACH_ALWAYS || stub_target_flash_needs_attach()) {
        STUB_LOGD("Attach spi flash...\n");
        stub_target_flash_attach(0, 0);
    }

    REG_SET_BIT(SPI_MEM_USER_REG(1), SPI_MEM_USR_COMMAND);
}
