/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/log.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>

#include <soc/dport_reg.h>
#include <soc/soc.h>

extern void Cache_Flush_rom(int cpu_num);
extern void Cache_Read_Disable_rom(int cpu_num);
extern void Cache_Read_Enable_rom(int cpu_num);

uint32_t stub_target_cache_get_caps(void)
{
    return STUB_CACHE_CAP_SHARED_IDCACHE;
}

void stub_target_cache_writeback_all(void)
{
    if (DPORT_REG_GET_BIT(DPORT_PRO_CACHE_CTRL1_REG, DPORT_PRO_CACHE_MASK_DRAM1)) {
        /* PSRAM not mapped - nothing to write back */
        return;
    }

    /*
     * ESP32 has no hardware writeback. Reading 64KB forces all dirty
     * lines to be written back to PSRAM (new reads replace them in the
     * 32KB 2-way set-associative cache).
     * LOWHIGH mode has two separate 32KB caches split at +2MB offset,
     * so we must read 64KB from each region.
     */

    /* Reads must go through cache to evict dirty lines; re-enable if caller suspended it */
    bool was_enabled = stub_target_cache_is_enabled();
    if (!was_enabled)
        Cache_Read_Enable_rom(0);

    volatile uint8_t *psram = (volatile uint8_t *)SOC_EXTRAM_DATA_LOW;
    volatile int i = 0;
    bool lowhigh = DPORT_REG_GET_BIT(DPORT_PRO_CACHE_CTRL_REG, DPORT_PRO_DRAM_HL);

    STUB_LOGD("Writing back all cache (lowhigh: %d)\n", lowhigh);

    for (int x = 0; x < 1024 * 64; x += 32) {
        i += psram[x];
        if (lowhigh)
            i += psram[x + (1024 * 1024 * 2)];
    }

    if (!was_enabled)
        Cache_Read_Disable_rom(0);
}

void stub_target_cache_invalidate_all(void)
{
    /* Flush invalidates all cache lines without writing back to memory */
    Cache_Flush_rom(0);
    Cache_Flush_rom(1);
}

void stub_target_cache_stop(void)
{
    Cache_Read_Disable_rom(0);
}

void stub_target_cache_start(void)
{
    Cache_Read_Enable_rom(0);
}

int stub_target_cache_is_enabled(void)
{
    return DPORT_REG_GET_BIT(DPORT_PRO_CACHE_CTRL_REG, DPORT_PRO_CACHE_ENABLE);
}

void stub_target_cache_init(void *state)
{
    (void)state;

    if (!stub_target_cache_is_enabled()) {
        STUB_LOGD("PRO cache not enabled, initializing for DROM0\n");
        Cache_Read_Disable_rom(0);
        Cache_Flush_rom(0);
        /* Disable all buses, then enable only DROM0 */
        uint32_t ctrl1 = DPORT_REG_READ(DPORT_PRO_CACHE_CTRL1_REG);
        ctrl1 |= DPORT_CACHE_BUS_MASK;
        ctrl1 &= ~DPORT_PRO_CACHE_MASK_DROM0;
        DPORT_REG_WRITE(DPORT_PRO_CACHE_CTRL1_REG, ctrl1);
        Cache_Read_Enable_rom(0);
    }
}
