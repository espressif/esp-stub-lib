/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/cache.h>
#include <esp-stub-lib/err.h>
#include <esp-stub-lib/flash.h>
#include <esp-stub-lib/log.h>

#include <target/mmu.h>

#define STUB_MMAP_MAX_PAGES 8

static struct {
    uint32_t vaddr_base;
    uint32_t entry_start;
    uint32_t page_count;
    uint32_t saved_entries[STUB_MMAP_MAX_PAGES];
} s_mmap_state;

static uint32_t mmu_page_shift(uint32_t page_size)
{
    switch (page_size) {
    case STUB_MMU_PAGE_SIZE_8KB:
        return 13U;
    case STUB_MMU_PAGE_SIZE_16KB:
        return 14U;
    case STUB_MMU_PAGE_SIZE_32KB:
        return 15U;
    case STUB_MMU_PAGE_SIZE_64KB:
        return 16U;
    case STUB_MMU_PAGE_SIZE_128KB:
        return 17U;
    case STUB_MMU_PAGE_SIZE_256KB:
        return 18U;
    default:
        STUB_LOGE("invalid page_size: %d\n", page_size);
        return 0;
    }
}

static void mmu_invalidate_region(uint32_t vaddr, uint32_t size)
{
    uint32_t caps = stub_lib_cache_get_caps();
    if (caps & STUB_CACHE_CAP_HAS_INVALIDATE_ADDR) {
        stub_lib_cache_invalidate_addr(vaddr, size);
    } else {
        /* invalidate_all drops ALL dirty lines (e.g. ESP32 Cache_Flush),
         * so writeback first to protect dirty PSRAM data */
        stub_lib_cache_writeback_all();
        stub_lib_cache_invalidate_all();
    }
}

/**
 * @brief Map a flash physical address range into the CPU virtual address space.
 *
 * Temporarily programs MMU entries so the CPU can read flash through cache.
 * Only one mapping can be active at a time. The caller must call
 * mmu_munmap() before creating a new mapping.
 *
 * @param flash_paddr  Flash physical address (page-aligned internally).
 * @param size         Number of bytes to map (rounded up to page boundary).
 * @param[out] out_vaddr  On success, receives the virtual address corresponding
 *                        to flash_paddr (including sub-page offset).
 *
 * @return STUB_LIB_OK on success, or a negative error code.
 */
static int mmu_mmap(uint32_t flash_paddr, uint32_t size, const void **out_vaddr)
{
    uint32_t page_size = stub_target_mmu_get_page_size();
    uint32_t page_shift = mmu_page_shift(page_size);
    uint32_t aligned = ALIGN_DOWN(flash_paddr, page_size);
    uint32_t offset = flash_paddr - aligned;

    if (size == 0 || page_shift == 0) {
        STUB_LOGE("invalid size: %d, or page_shift: %d\n", size, page_shift);
        return STUB_LIB_ERR_INVALID_ARG;
    }

    if (size > UINT32_MAX - offset) {
        STUB_LOGE("size %d + offset %d overflows uint32_t\n", size, offset);
        return STUB_LIB_ERR_INVALID_ARG;
    }

    uint32_t map_size = size + offset;
    uint32_t page_count = (map_size + page_size - 1) >> page_shift;

    STUB_LOGD("page_size: %d, page_shift: %d page_count: %d\n", page_size, page_shift, page_count);
    STUB_LOGD("aligned: %x, offset: %x, map_size: %x\n", aligned, offset, map_size);

    if (page_count > STUB_MMAP_MAX_PAGES) {
        STUB_LOGE("invalid page_count: %d\n", page_count);
        return STUB_LIB_ERR_INVALID_ARG;
    }

    uint32_t drom_start = stub_target_mmu_get_drom_entry_start();
    uint32_t drom_end = stub_target_mmu_get_drom_entry_end();
    uint32_t drom_count = drom_end - drom_start;
    if (page_count > drom_count) {
        STUB_LOGE("invalid page_count: %d, drom_count: %d\n", page_count, drom_count);
        return STUB_LIB_ERR_INVALID_ARG;
    }

    /* Use the last N entries of the DROM region (least likely to conflict) */
    uint32_t entry_start = drom_end - page_count;
    uint32_t drom_vaddr = stub_target_mmu_get_drom_vaddr();
    uint32_t vaddr_base = drom_vaddr + (entry_start - drom_start) * page_size;

    STUB_LOGD("drom_start: %d, drom_end: %d, entry_start: %d, vaddr_base: %x\n",
              drom_start,
              drom_end,
              entry_start,
              vaddr_base);

    stub_lib_cache_stop();

    /* Save existing MMU entries */
    for (uint32_t i = 0; i < page_count; i++) {
        s_mmap_state.saved_entries[i] = stub_target_mmu_read_entry(entry_start + i);
    }

    /* Write new flash mappings */
    uint32_t flash_page = aligned >> page_shift;
    for (uint32_t i = 0; i < page_count; i++) {
        stub_target_mmu_write_entry(entry_start + i, flash_page + i);
    }

    s_mmap_state.vaddr_base = vaddr_base;
    s_mmap_state.entry_start = entry_start;
    s_mmap_state.page_count = page_count;

    mmu_invalidate_region(vaddr_base, page_count * page_size);

    stub_lib_cache_start();

    *out_vaddr = (const void *)(uintptr_t)(vaddr_base + offset);

    STUB_LOGD("mmap: 0x%x is mapped to 0x%x\n", flash_paddr, (uint32_t)*out_vaddr);

    return STUB_LIB_OK;
}

/**
 * @brief Unmap a previously mapped flash region and restore MMU state.
 */
static void mmu_munmap(void)
{
    if (s_mmap_state.page_count == 0)
        return;

    uint32_t page_size = stub_target_mmu_get_page_size();
    stub_lib_cache_stop();

    /* Restore saved MMU entries */
    for (uint32_t i = 0; i < s_mmap_state.page_count; i++) {
        stub_target_mmu_restore_entry(s_mmap_state.entry_start + i, s_mmap_state.saved_entries[i]);
    }

    mmu_invalidate_region(s_mmap_state.vaddr_base, s_mmap_state.page_count * page_size);

    stub_lib_cache_start();

    s_mmap_state.page_count = 0;
}

int stub_lib_mmu_read_flash(uint32_t addr, void *buffer, uint32_t size)
{
    const void *vaddr;
    int rc = mmu_mmap(addr, size, &vaddr);
    if (rc != STUB_LIB_OK) {
        STUB_LOGE("mmap failed (%d) @ %x\n", rc, addr);
        return rc;
    }

    memcpy(buffer, vaddr, size);

    mmu_munmap();

    return STUB_LIB_OK;
}
