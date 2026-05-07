/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stddef.h>
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

void __attribute__((weak)) stub_target_cache_stop(void)
{
}

void __attribute__((weak)) stub_target_cache_start(void)
{
}

size_t __attribute__((weak)) stub_target_cache_state_size(void)
{
    return 0;
}

void __attribute__((weak)) stub_target_cache_init(void *state)
{
    (void)state;
}

void __attribute__((weak)) stub_target_cache_deinit(const void *state)
{
    (void)state;
}

int __attribute__((weak)) stub_target_cache_is_enabled(void)
{
    return 0;
}
