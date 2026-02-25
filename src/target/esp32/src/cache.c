/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/log.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>

#include <soc/dport_reg.h>

extern void Cache_Flush_rom(int cpu_num);
extern void Cache_Read_Disable_rom(int cpu_num);
extern void Cache_Read_Enable_rom(int cpu_num);

#define CACHE_LINE_SIZE 32UL

void stub_target_cache_writeback_all(void)
{
    // TODO: Take care of PSRAM writeback.
}

void stub_target_cache_invalidate_all(void)
{
    /* Flush invalidates all cache lines but does not write back to PSRAM. */
    Cache_Flush_rom(0);
    Cache_Flush_rom(1);
}

/* State packing: BIT(0) = PRO cache enabled, BIT(1) = APP cache enabled */
uint32_t stub_target_cache_suspend(void)
{
    uint32_t state = 0;

    if (DPORT_REG_GET_BIT(DPORT_PRO_CACHE_CTRL_REG, DPORT_PRO_CACHE_ENABLE)) {
        state |= BIT(0);
        Cache_Read_Disable_rom(0);
    }

    if (DPORT_REG_GET_BIT(DPORT_APP_CACHE_CTRL_REG, DPORT_APP_CACHE_ENABLE)) {
        state |= BIT(1);
        Cache_Read_Disable_rom(1);
    }

    return state;
}

void stub_target_cache_resume(uint32_t autoload)
{
    if (autoload & BIT(0)) {
        Cache_Read_Enable_rom(0);
    }

    if (autoload & BIT(1)) {
        Cache_Read_Enable_rom(1);
    }
}
