/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <target/cache.h>

extern void Cache_Invalidate_ICache_All(void);
extern uint32_t Cache_Disable_ICache(void);
extern void Cache_Enable_ICache(uint32_t autoload);

void __attribute__((weak)) stub_target_cache_invalidate_all(void)
{
    Cache_Invalidate_ICache_All();
}

void __attribute__((weak)) stub_target_cache_writeback_invalidate_all(void)
{
    stub_target_cache_invalidate_all();
}

void __attribute__((weak)) stub_target_cache_disable(void)
{
    Cache_Disable_ICache();
}

void __attribute__((weak)) stub_target_cache_enable(void)
{
    Cache_Enable_ICache(0);
}
