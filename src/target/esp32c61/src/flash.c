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
#include <target/clock.h>
#include <target/flash.h>

#include <soc/pcr_reg.h>
#include <soc/spi_mem_compat.h>

extern uint32_t esp_rom_get_xtal_freq(void);

void stub_target_spi_wait_ready(void)
{
    while (REG_GET_FIELD(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_MST_ST) ||
           REG_GET_FIELD(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_SLV_ST)) {
        /* busy wait */
    }
}

uint32_t stub_target_get_max_supported_flash_size(void)
{
    /* ESP32-C61 supports up to 32MB with 4-byte addressing */
    return MIB(32);
}

uint32_t stub_target_flash_get_src_freq_mhz(void)
{
    /* XTAL, RC_FAST or the 480 MHz PLL, the unused value as the fastest source */
    static const uint16_t src_mhz[] = { 17, 480, 480 };
    uint32_t sel = REG_GET_FIELD(PCR_MSPI_CLK_CONF_REG, PCR_MSPI_FUNC_CLK_SEL);
    uint32_t src = sel == 0 ? esp_rom_get_xtal_freq() / MHZ : src_mhz[sel - 1];
    return src / (REG_GET_FIELD(PCR_MSPI_CLK_CONF_REG, PCR_MSPI_FAST_DIV_NUM) + 1);
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
