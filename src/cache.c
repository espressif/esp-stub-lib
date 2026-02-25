/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stddef.h>

#include <target/cache.h>

void stub_lib_cache_save(const void **state_out)
{
    if (state_out == NULL) {
        return;
    }

    stub_target_cache_save(state_out);
}

void stub_lib_cache_restore(const void *state)
{
    if (state == NULL) {
        return;
    }

    stub_target_cache_restore(state);
}
