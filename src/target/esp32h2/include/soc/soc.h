/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#define SOC_MMU_PAGE_SIZE 0x8000

/* Overall memory map */
#define SOC_IROM_LOW 0x42000000
#define SOC_IROM_HIGH (SOC_IROM_LOW + (SOC_MMU_PAGE_SIZE<<8))
#define SOC_DROM_LOW SOC_IROM_LOW
#define SOC_DROM_HIGH SOC_IROM_HIGH
#define SOC_IROM_MASK_LOW 0x40000000
#define SOC_IROM_MASK_HIGH 0x40020000
#define SOC_DROM_MASK_LOW 0x40000000
#define SOC_DROM_MASK_HIGH 0x40020000
#define SOC_IRAM_LOW 0x40800000
#define SOC_IRAM_HIGH 0x40850000
#define SOC_DRAM_LOW 0x40800000
#define SOC_DRAM_HIGH 0x40850000
#define SOC_RTC_IRAM_LOW 0x50000000 // ESP32-H2 only has 4k LP memory
#define SOC_RTC_IRAM_HIGH 0x50001000
#define SOC_RTC_DRAM_LOW 0x50000000
#define SOC_RTC_DRAM_HIGH 0x50001000
#define SOC_RTC_DATA_LOW 0x50000000
#define SOC_RTC_DATA_HIGH 0x50001000

//First and last words of the D/IRAM region, for both the DRAM address as well as the IRAM alias.
#define SOC_DIRAM_IRAM_LOW 0x40800000
#define SOC_DIRAM_IRAM_HIGH 0x40850000
#define SOC_DIRAM_DRAM_LOW 0x40800000
#define SOC_DIRAM_DRAM_HIGH 0x40850000

#define MAP_DRAM_TO_IRAM(addr) (addr)
#define MAP_IRAM_TO_DRAM(addr) (addr)

// Region of memory accessible via DMA. See esp_ptr_dma_capable().
#define SOC_DMA_LOW 0x40800000
#define SOC_DMA_HIGH 0x40850000

// Region of RAM that is byte-accessible. See esp_ptr_byte_accessible().
#define SOC_BYTE_ACCESSIBLE_LOW 0x40800000
#define SOC_BYTE_ACCESSIBLE_HIGH 0x40850000

//Region of memory that is internal, as in on the same silicon die as the ESP32 CPUs
//(excluding RTC data region, that's checked separately.) See esp_ptr_internal().
#define SOC_MEM_INTERNAL_LOW 0x40800000
#define SOC_MEM_INTERNAL_HIGH 0x40850000
#define SOC_MEM_INTERNAL_LOW1 0x40800000
#define SOC_MEM_INTERNAL_HIGH1 0x40850000

#define SOC_MAX_CONTIGUOUS_RAM_SIZE (SOC_IRAM_HIGH - SOC_IRAM_LOW) ///< Largest span of contiguous memory (DRAM or IRAM) in the address space

// Region of address space that holds peripherals
#define SOC_PERIPHERAL_LOW 0x60000000
#define SOC_PERIPHERAL_HIGH 0x60100000

// CPU sub-system region, contains interrupt config registers
#define SOC_CPU_SUBSYSTEM_LOW 0x20000000
#define SOC_CPU_SUBSYSTEM_HIGH 0x30000000

// Start (highest address) of ROM boot stack, only relevant during early boot
#define SOC_ROM_STACK_START 0x4084f380
#define SOC_ROM_STACK_SIZE 0x2000
