/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <target/cache.h>

extern void Cache_Invalidate_ICache_All(void);

void __attribute__((weak)) stub_target_cache_invalidate_all(void)
{
    Cache_Invalidate_ICache_All();
}

void __attribute__((weak)) stub_target_cache_writeback_invalidate_all(void)
{
    stub_target_cache_invalidate_all();
}
