/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>

#include <esp-stub-lib/log.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>
#include <target/flash.h>

#include <soc/spi_mem_compat.h>

extern void esp_rom_spiflash_attach(uint32_t ishspi, bool legacy);
extern void spi_common_set_flash_cs_timing(void);

typedef struct {
    uint32_t ctrl;
} esp32s31_flash_state_t;

static esp32s31_flash_state_t s_flash_state;

void stub_target_spi_wait_ready(void)
{
    while (REG_GET_FIELD(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_MST_ST) ||
           REG_GET_FIELD(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_SLV_ST)) {
        /* busy wait */
    }
}

bool stub_target_flash_needs_attach(void)
{
    return !stub_target_cache_is_enabled();
}

void stub_target_flash_state_save(void **state)
{
    if (!state)
        return;

    s_flash_state.ctrl = READ_PERI_REG(SPI_MEM_CTRL_REG(FLASH_SPI_NUM));
    *state = &s_flash_state;
}

void stub_target_flash_state_restore(const void *state)
{
    if (!state)
        return;

    const esp32s31_flash_state_t *s = state;
    WRITE_PERI_REG(SPI_MEM_CTRL_REG(FLASH_SPI_NUM), s->ctrl);
}

void stub_target_flash_init(void **state, stub_lib_flash_attach_policy_t attach_policy)
{
    if (state)
        stub_target_flash_state_save(state);

    if (attach_policy == STUB_LIB_FLASH_ATTACH_ALWAYS || stub_target_flash_needs_attach()) {
        STUB_LOGD("Attach spi flash...\n");
        esp_rom_spiflash_attach(0, false);
    } else {
        /*
         * Skip esp_rom_spiflash_attach() when cache is active (MMU entries live).
         * Reproduce the minimum SPI1 setup from ROM's SPI_init():
         * - SYNC_RESET: clears a stuck MSPI state machine
         * - Clear FLASH_SUS: drops any suspend status that could block erase/program
         * - Write CTRL with WP and RESANDRES: drives /WP high to release hardware write protect
         * - Set CS timing via ROM helper
         */
        REG_SET_BIT(SPI_MEM_CTRL2_REG(FLASH_SPI_NUM), SPI_MEM_SYNC_RESET);
        CLEAR_PERI_REG_MASK(SPI_MEM_SUS_STATUS_REG(FLASH_SPI_NUM), SPI_MEM_FLASH_SUS_M);
        WRITE_PERI_REG(SPI_MEM_CTRL_REG(FLASH_SPI_NUM), SPI_MEM_WP_REG | SPI_MEM_RESANDRES);
        spi_common_set_flash_cs_timing();
    }

    REG_SET_BIT(SPI_MEM_USER_REG(FLASH_SPI_NUM), SPI_MEM_USR_COMMAND);
}
