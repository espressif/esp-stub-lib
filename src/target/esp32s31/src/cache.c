/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/log.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>
#include <target/mmu.h>

#include <soc/cache_reg.h>

#define CACHE_MAP_L1_ICACHE_0 BIT(0)
#define CACHE_MAP_L1_ICACHE_1 BIT(1)
#define CACHE_MAP_L1_DCACHE   BIT(4)
#define CACHE_MAP_L1_ICACHE   (CACHE_MAP_L1_ICACHE_0 | CACHE_MAP_L1_ICACHE_1)
#define CACHE_MAP_ALL         (CACHE_MAP_L1_ICACHE | CACHE_MAP_L1_DCACHE)

extern int Cache_Invalidate_All(uint32_t map);
extern int Cache_Invalidate_Addr(uint32_t map, uint32_t addr, uint32_t size);
extern void Cache_WriteBack_All(uint32_t map);
extern int Cache_WriteBack_Addr(uint32_t map, uint32_t addr, uint32_t size);

extern void ROM_Boot_Cache_Init(void);
extern uint32_t Cache_Disable_L1_DCache(void);

/* Freeze API is reached through a ROM-resident function table on S31.
 * Layout must match esp_rom/esp32s31/include/esp32s31/rom/cache.h
 * `struct cache_internal_stub_table`. Only the freeze members are used
 * here; the rest are kept solely to preserve member offsets. */
#define CACHE_FREEZE_ACK_BUSY 0

struct cache_internal_stub_table {
    uint32_t (*l1_icache_line_size)(void);
    uint32_t (*l1_dcache_line_size)(void);
    uint32_t (*cache_addr)(uint32_t addr);
    void (*sync_cache_items)(uint32_t type, uint32_t map, uint32_t addr, uint32_t bytes);
    void (*lock_cache_items)(uint32_t lock, uint32_t map, uint32_t addr, uint32_t bytes);
    uint32_t (*suspend_l1_icache0_autoload)(void);
    void (*resume_l1_icache0_autoload)(uint32_t autoload);
    uint32_t (*suspend_l1_icache1_autoload)(void);
    void (*resume_l1_icache1_autoload)(uint32_t autoload);
    uint32_t (*suspend_l1_dcache_autoload)(void);
    void (*resume_l1_dcache_autoload)(uint32_t autoload);
    void (*freeze_l1_icache0_enable)(int mode);
    void (*freeze_l1_icache0_disable)(void);
    void (*freeze_l1_icache1_enable)(int mode);
    void (*freeze_l1_icache1_disable)(void);
    void (*freeze_l1_dcache_enable)(int mode);
    void (*freeze_l1_dcache_disable)(void);
    int (*op_addr)(uint32_t op_type,
                   uint32_t map,
                   uint32_t start_addr,
                   uint32_t size,
                   uint32_t cache_line_size,
                   uint32_t max_sync_num,
                   void (*cache_op)(uint32_t, uint32_t, uint32_t, uint32_t));
};

extern const struct cache_internal_stub_table *rom_cache_internal_table_ptr;

uint32_t stub_target_cache_get_caps(void)
{
    return STUB_CACHE_CAP_HAS_INVALIDATE_ADDR | STUB_CACHE_CAP_SHARED_IDCACHE;
}

void stub_target_cache_writeback_addr(uint32_t vaddr, uint32_t size)
{
    Cache_WriteBack_Addr(CACHE_MAP_L1_DCACHE, vaddr, size);
}

void stub_target_cache_writeback_all(void)
{
    Cache_WriteBack_All(CACHE_MAP_ALL);
}

void stub_target_cache_invalidate_all(void)
{
    Cache_Invalidate_All(CACHE_MAP_ALL);
}

void stub_target_cache_invalidate_addr(uint32_t vaddr, uint32_t size)
{
    Cache_Invalidate_Addr(CACHE_MAP_L1_ICACHE, vaddr, size);
    Cache_Invalidate_Addr(CACHE_MAP_L1_DCACHE, vaddr, size);
}

void stub_target_cache_stop(void)
{
    rom_cache_internal_table_ptr->freeze_l1_icache0_enable(CACHE_FREEZE_ACK_BUSY);
    rom_cache_internal_table_ptr->freeze_l1_icache1_enable(CACHE_FREEZE_ACK_BUSY);
    rom_cache_internal_table_ptr->freeze_l1_dcache_enable(CACHE_FREEZE_ACK_BUSY);
}

void stub_target_cache_start(void)
{
    rom_cache_internal_table_ptr->freeze_l1_dcache_disable();
    rom_cache_internal_table_ptr->freeze_l1_icache1_disable();
    rom_cache_internal_table_ptr->freeze_l1_icache0_disable();
}

int stub_target_cache_is_enabled(void)
{
    uint32_t icache_ctrl = REG_READ(CACHE_L1_ICACHE_CTRL_REG);
    if (icache_ctrl & (CACHE_L1_ICACHE_SHUT_IBUS0 | CACHE_L1_ICACHE_SHUT_IBUS1))
        return 0;

    uint32_t dcache_ctrl = REG_READ(CACHE_L1_DCACHE_CTRL_REG);
    if (dcache_ctrl & (CACHE_L1_DCACHE_SHUT_DBUS0 | CACHE_L1_DCACHE_SHUT_DBUS1))
        return 0;

    return stub_target_mmu_has_valid_entry();
}

void stub_target_cache_init(void *state)
{
    (void)state;

    bool cache_was_enabled = stub_target_cache_is_enabled();
    if (!cache_was_enabled) {
        STUB_LOGD("DCache not enabled, initializing for DROM\n");
        ROM_Boot_Cache_Init();
    }
}
