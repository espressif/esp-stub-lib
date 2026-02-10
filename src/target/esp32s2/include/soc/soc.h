/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

/* Overall memory map */
#define SOC_DROM_LOW 0x3F000000/*drom0 low address for icache*/
#define SOC_DROM_HIGH 0x3FF80000/*dram0 high address for dcache*/
#define SOC_IROM_LOW 0x40080000
#define SOC_IROM_HIGH 0x40800000
#define SOC_IROM_MASK_LOW 0x40000000
#define SOC_IROM_MASK_HIGH 0x40020000
#define SOC_IRAM_LOW 0x40020000
#define SOC_IRAM_HIGH 0x40070000
#define SOC_DRAM_LOW 0x3FFB0000
#define SOC_DRAM_HIGH 0x40000000
#define SOC_RTC_IRAM_LOW 0x40070000
#define SOC_RTC_IRAM_HIGH 0x40072000
#define SOC_RTC_DRAM_LOW 0x3ff9e000
#define SOC_RTC_DRAM_HIGH 0x3ffa0000
#define SOC_RTC_DATA_LOW 0x50000000
#define SOC_RTC_DATA_HIGH 0x50002000
#define SOC_EXTRAM_DATA_LOW 0x3F500000
#define SOC_EXTRAM_DATA_HIGH 0x3FF80000

#define SOC_EXTRAM_DATA_SIZE (SOC_EXTRAM_DATA_HIGH - SOC_EXTRAM_DATA_LOW)

//First and last words of the D/IRAM region, for both the DRAM address as well as the IRAM alias.
#define SOC_DIRAM_IRAM_LOW 0x40020000
#define SOC_DIRAM_IRAM_HIGH 0x40070000
#define SOC_DIRAM_DRAM_LOW 0x3FFB0000
#define SOC_DIRAM_DRAM_HIGH 0x40000000

#define SOC_I_D_OFFSET (SOC_DIRAM_IRAM_LOW - SOC_DIRAM_DRAM_LOW)
#define MAP_DRAM_TO_IRAM(addr) (addr + SOC_I_D_OFFSET)
#define MAP_IRAM_TO_DRAM(addr) (addr - SOC_I_D_OFFSET)

// Region of memory accessible via DMA in internal memory. See esp_ptr_dma_capable().
#define SOC_DMA_LOW 0x3FFB0000
#define SOC_DMA_HIGH 0x40000000

// Region of memory accessible via DMA in external memory. See esp_ptr_dma_ext_capable().
#define SOC_DMA_EXT_LOW 0x3F500000
#define SOC_DMA_EXT_HIGH 0x3FF80000

// Region of memory that is byte-accessible. See esp_ptr_byte_accessible().
#define SOC_BYTE_ACCESSIBLE_LOW 0x3FF9E000
#define SOC_BYTE_ACCESSIBLE_HIGH 0x40000000

//Region of memory that is internal, as in on the same silicon die as the ESP32 CPUs
//(excluding RTC data region, that's checked separately.) See esp_ptr_internal().
#define SOC_MEM_INTERNAL_LOW 0x3FF9E000
#define SOC_MEM_INTERNAL_HIGH 0x40072000

// Start (highest address) of ROM boot stack, only relevant during early boot
#define SOC_ROM_STACK_START 0x3fffe70c
