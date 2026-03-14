/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>

#include <soc/dport_reg.h>
#include <soc/soc.h>

extern void Cache_Flush_rom(int cpu_num);
extern void Cache_Read_Disable_rom(int cpu_num);
extern void Cache_Read_Enable_rom(int cpu_num);

void stub_target_cache_writeback_all(void)
{
    /*
    Note: this assumes the amount of external RAM is >2M. If it is 2M or less, what this code does is undefined. If
    we ever support external RAM chips of 2M or smaller, this may need adjusting.
    */

    int x;
    volatile int i = 0;
    volatile uint8_t *psram = (volatile uint8_t *)SOC_EXTRAM_DATA_LOW;
    for (x = 0; x < 1024 * 64; x += 32) {
        i += psram[x];
        i += psram[x + (1024 * 1024 * 2)];
    }
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
