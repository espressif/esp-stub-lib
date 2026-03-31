/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <target/cache.h>

uint32_t __attribute__((weak)) stub_target_cache_get_caps(void)
{
    return 0;
}

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
}

void __attribute__((weak)) stub_target_cache_writeback_addr(uint32_t vaddr, uint32_t size)
{
    (void)vaddr;
    (void)size;
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

int __attribute__((weak)) stub_target_cache_is_enabled(void)
{
    return 0;
}
