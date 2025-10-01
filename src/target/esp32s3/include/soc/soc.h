/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

/* Overall memory map */
#define SOC_DROM_LOW 0x3C000000
#define SOC_DROM_HIGH 0x3E000000
#define SOC_IROM_LOW 0x42000000
#define SOC_IROM_HIGH 0x44000000
#define SOC_IRAM_LOW 0x40370000
#define SOC_IRAM_HIGH 0x403E0000
#define SOC_DRAM_LOW 0x3FC88000
#define SOC_DRAM_HIGH 0x3FD00000

#define SOC_RTC_IRAM_LOW 0x600FE000
#define SOC_RTC_IRAM_HIGH 0x60100000
#define SOC_RTC_DRAM_LOW 0x600FE000
#define SOC_RTC_DRAM_HIGH 0x60100000

#define SOC_RTC_DATA_LOW 0x50000000
#define SOC_RTC_DATA_HIGH 0x50002000

#define SOC_EXTRAM_DATA_LOW 0x3C000000
#define SOC_EXTRAM_DATA_HIGH 0x3E000000
#define SOC_IROM_MASK_LOW 0x40000000
#define SOC_IROM_MASK_HIGH 0x40060000

#define SOC_EXTRAM_DATA_SIZE (SOC_EXTRAM_DATA_HIGH - SOC_EXTRAM_DATA_LOW)
#define SOC_MAX_CONTIGUOUS_RAM_SIZE (SOC_EXTRAM_DATA_HIGH - SOC_EXTRAM_DATA_LOW) ///< Largest span of contiguous memory (DRAM or IRAM) in the address space

//First and last words of the D/IRAM region, for both the DRAM address as well as the IRAM alias.
#define SOC_DIRAM_IRAM_LOW 0x40378000
#define SOC_DIRAM_IRAM_HIGH 0x403E0000
#define SOC_DIRAM_DRAM_LOW 0x3FC88000
#define SOC_DIRAM_DRAM_HIGH 0x3FCF0000

#define SOC_I_D_OFFSET (SOC_DIRAM_IRAM_LOW - SOC_DIRAM_DRAM_LOW)
#define MAP_DRAM_TO_IRAM(addr) (addr + SOC_I_D_OFFSET)
#define MAP_IRAM_TO_DRAM(addr) (addr - SOC_I_D_OFFSET)

// Region of memory accessible via DMA in internal memory. See esp_ptr_dma_capable().
#define SOC_DMA_LOW 0x3FC88000
#define SOC_DMA_HIGH 0x3FD00000

// Region of memory accessible via DMA in external memory. See esp_ptr_dma_ext_capable().
#define SOC_DMA_EXT_LOW SOC_EXTRAM_DATA_LOW
#define SOC_DMA_EXT_HIGH SOC_EXTRAM_DATA_HIGH

// Region of memory that is byte-accessible. See esp_ptr_byte_accessible().
#define SOC_BYTE_ACCESSIBLE_LOW 0x3FC88000
#define SOC_BYTE_ACCESSIBLE_HIGH 0x3FD00000

//Region of memory that is internal, as in on the same silicon die as the ESP32 CPUs
//(excluding RTC data region, that's checked separately.) See esp_ptr_internal().
#define SOC_MEM_INTERNAL_LOW 0x3FC88000
#define SOC_MEM_INTERNAL_HIGH 0x403E0000

// Start (highest address) of ROM boot stack, only relevant during early boot
#define SOC_ROM_STACK_START 0x3fceb710
#define SOC_ROM_STACK_SIZE 0x2000
