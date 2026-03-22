/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stddef.h>

#include <target/cache.h>

uint32_t stub_lib_cache_get_caps(void)
{
    return stub_target_cache_get_caps();
}

void stub_lib_cache_writeback_all(void)
{
    stub_target_cache_writeback_all();
}

void stub_lib_cache_invalidate_all(void)
{
    stub_target_cache_invalidate_all();
}

void stub_lib_cache_invalidate_addr(uint32_t vaddr, uint32_t size)
{
    stub_target_cache_invalidate_addr(vaddr, size);
}

void stub_lib_cache_writeback_addr(uint32_t vaddr, uint32_t size)
{
    stub_target_cache_writeback_addr(vaddr, size);
}

uint32_t stub_lib_cache_suspend(void)
{
    return stub_target_cache_suspend();
}

void stub_lib_cache_resume(uint32_t autoload)
{
    stub_target_cache_resume(autoload);
}

void stub_lib_cache_save(void)
{
    stub_target_cache_save();
}

void stub_lib_cache_restore(void)
{
    stub_target_cache_restore();
}
