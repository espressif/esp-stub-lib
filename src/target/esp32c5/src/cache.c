/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>

#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>

#include <soc/cache_reg.h>
#include <soc/spi_mem_compat.h>

extern void Cache_Invalidate_All(void);
extern void Cache_WriteBack_All(void);
extern uint32_t Cache_Suspend_Cache(void);
extern void Cache_Resume_Cache(uint32_t autoload);

typedef struct {
    bool autoload_enabled;
} esp32c5_cache_state_t;

static esp32c5_cache_state_t s_cache_state;

void stub_target_cache_save(const void **state_out)
{
    /* On esp32c5, MMU Page size is always 64KB. No need to save it. */

    Cache_WriteBack_All();
    s_cache_state.autoload_enabled = Cache_Suspend_Cache();

    *state_out = &s_cache_state;
}

void stub_target_cache_restore(const void *state)
{
    const esp32c5_cache_state_t *s = state;

    /* Invalidate stale lines from flash writes */
    Cache_Invalidate_All();
    Cache_Resume_Cache(s->autoload_enabled ? CACHE_L1_CACHE_AUTOLOAD_ENA : 0U);
}
