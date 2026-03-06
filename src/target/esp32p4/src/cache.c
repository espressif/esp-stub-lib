/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>

#define CACHE_MAP_L1_ICACHE_0 BIT(0)
#define CACHE_MAP_L1_ICACHE_1 BIT(1)
#define CACHE_MAP_L1_DCACHE   BIT(4)
#define CACHE_MAP_L2_CACHE    BIT(5)
#define CACHE_MAP_L1_ICACHE   (CACHE_MAP_L1_ICACHE_0 | CACHE_MAP_L1_ICACHE_1)
#define CACHE_MAP_ALL         (CACHE_MAP_L1_ICACHE | CACHE_MAP_L1_DCACHE | CACHE_MAP_L2_CACHE)

extern int Cache_Invalidate_All(uint32_t map);
extern int Cache_WriteBack_All(uint32_t map);

extern uint32_t Cache_Suspend_L1_CORE0_ICache_Autoload(void);
extern void Cache_Resume_L1_CORE0_ICache_Autoload(uint32_t autoload);
extern uint32_t Cache_Suspend_L1_CORE1_ICache_Autoload(void);
extern void Cache_Resume_L1_CORE1_ICache_Autoload(uint32_t autoload);

extern uint32_t Cache_Suspend_L1_CORE1_ICache(void);
extern void Cache_Resume_L1_CORE1_ICache(uint32_t autoload);

extern uint32_t Cache_Suspend_L1_DCache_Autoload(void);
extern void Cache_Resume_L1_DCache_Autoload(uint32_t autoload);
extern uint32_t Cache_Suspend_L2_Cache(void);
extern void Cache_Resume_L2_Cache(uint32_t autoload);

typedef struct {
    uint32_t autoload_icache0;
    uint32_t autoload_icache1;
    uint32_t autoload_dcache;
    uint32_t autoload_l2cache;
} esp32p4_cache_state_t;

static esp32p4_cache_state_t s_cache_state;

void stub_target_cache_save(const void **state_out)
{
    /* Full L1-ICache0 suspend will block instruction fetch.
     * Since the stub runs from the L1-cached region, this causes a PMP fault.
     * We will suspend autoload only for ICache0.
     * ICache1 can be fully suspended safely.
     */
    s_cache_state.autoload_icache0 = Cache_Suspend_L1_CORE0_ICache_Autoload();
    s_cache_state.autoload_icache1 = Cache_Suspend_L1_CORE1_ICache();

    Cache_WriteBack_All(CACHE_MAP_L1_DCACHE | CACHE_MAP_L2_CACHE);

    s_cache_state.autoload_dcache = Cache_Suspend_L1_DCache_Autoload();
    s_cache_state.autoload_l2cache = Cache_Suspend_L2_Cache();

    *state_out = &s_cache_state;
}

void stub_target_cache_restore(const void *state)
{
    const esp32p4_cache_state_t *s = state;

    /* Invalidate stale lines from flash writes.
     * Resume L1 autoloads after L2 so they can refill from external memory. */
    Cache_Invalidate_All(CACHE_MAP_ALL);
    Cache_Resume_L2_Cache(s->autoload_l2cache);
    Cache_Resume_L1_DCache_Autoload(s->autoload_dcache);
    Cache_Resume_L1_CORE0_ICache_Autoload(s->autoload_icache0);
    Cache_Resume_L1_CORE1_ICache(s->autoload_icache1);
}
