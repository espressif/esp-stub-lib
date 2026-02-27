/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <cache.h>

#include <target/cache.h>

void stub_lib_cache_writeback_invalidate_all(void)
{
    stub_target_cache_writeback_invalidate_all();
}

void stub_lib_cache_invalidate_all(void)
{
    stub_target_cache_invalidate_all();
}

void stub_lib_cache_disable(void)
{
    stub_target_cache_disable();
}

void stub_lib_cache_enable(void)
{
    stub_target_cache_enable();
}
