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

void stub_lib_cache_stop(void)
{
    stub_target_cache_stop();
}

void stub_lib_cache_start(void)
{
    stub_target_cache_start();
}

size_t stub_lib_cache_state_size(void)
{
    return stub_target_cache_state_size();
}

void stub_lib_cache_init(void *state)
{
    stub_target_cache_init(state);
}

void stub_lib_cache_deinit(const void *state)
{
    stub_target_cache_deinit(state);
}

int stub_lib_is_cache_enabled(void)
{
    return stub_target_cache_is_enabled();
}
