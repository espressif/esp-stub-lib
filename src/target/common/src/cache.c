/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <target/cache.h>

void __attribute__((weak)) stub_target_cache_save(const void **state_out)
{
    (void)state_out;
}

void __attribute__((weak)) stub_target_cache_restore(const void *state)
{
    (void)state;
}
