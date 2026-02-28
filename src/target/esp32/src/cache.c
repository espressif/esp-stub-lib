/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <target/cache.h>

extern void Cache_Flush_rom(int cpu_num);

void stub_target_cache_writeback_invalidate_all(void)
{
    Cache_Flush_rom(0);
    Cache_Flush_rom(1);
}

void stub_target_cache_invalidate_all(void)
{
    Cache_Flush_rom(0);
    Cache_Flush_rom(1);
}
