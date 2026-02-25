/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <target/cache.h>

void __attribute__((weak)) stub_target_cache_writeback_all(void)
{
}

void __attribute__((weak)) stub_target_cache_invalidate_all(void)
{
}

void __attribute__((weak)) stub_target_cache_invalidate_addr(uint32_t vaddr, uint32_t size)
{
    (void)vaddr;
    (void)size;
    stub_target_cache_invalidate_all();
}

void __attribute__((weak)) stub_target_cache_writeback_addr(uint32_t vaddr, uint32_t size)
{
    (void)vaddr;
    (void)size;
    stub_target_cache_writeback_all();
}

uint32_t __attribute__((weak)) stub_target_cache_suspend(void)
{
    return 0;
}

void __attribute__((weak)) stub_target_cache_resume(uint32_t autoload)
{
    (void)autoload;
}

void __attribute__((weak)) stub_target_cache_save(void)
{
}

void __attribute__((weak)) stub_target_cache_restore(void)
{
}
