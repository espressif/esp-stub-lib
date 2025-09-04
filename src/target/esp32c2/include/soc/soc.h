/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

/* Overall memory map */
#define SOC_DROM_LOW 0x3C000000
#define SOC_DROM_HIGH 0x3C400000
#define SOC_IROM_LOW 0x42000000
#define SOC_IROM_HIGH 0x42400000
#define SOC_IROM_MASK_LOW 0x40000000
#define SOC_IROM_MASK_HIGH 0x40090000
#define SOC_DROM_MASK_LOW 0x3FF00000
#define SOC_DROM_MASK_HIGH 0x3FF50000
#define SOC_IRAM_LOW 0x4037C000
#define SOC_IRAM_HIGH 0x403C0000
#define SOC_DRAM_LOW 0x3FCA0000
#define SOC_DRAM_HIGH 0x3FCE0000

//First and last words of the D/IRAM region, for both the DRAM address as well as the IRAM alias.
#define SOC_DIRAM_IRAM_LOW 0x40380000
#define SOC_DIRAM_IRAM_HIGH 0x403C0000
#define SOC_DIRAM_DRAM_LOW 0x3FCA0000
#define SOC_DIRAM_DRAM_HIGH 0x3FCE0000

#define SOC_I_D_OFFSET (SOC_DIRAM_IRAM_LOW - SOC_DIRAM_DRAM_LOW)
#define MAP_DRAM_TO_IRAM(addr) (addr + SOC_I_D_OFFSET)
#define MAP_IRAM_TO_DRAM(addr) (addr - SOC_I_D_OFFSET)

// Region of memory accessible via DMA. See esp_ptr_dma_capable().
#define SOC_DMA_LOW 0x3FCA0000
#define SOC_DMA_HIGH 0x3FCE0000

// Region of RAM that is byte-accessible. See esp_ptr_byte_accessible().
#define SOC_BYTE_ACCESSIBLE_LOW 0x3FCA0000
#define SOC_BYTE_ACCESSIBLE_HIGH 0x3FCE0000

//Region of memory that is internal, as in on the same silicon die as the ESP32 CPUs
//(excluding RTC data region, that's checked separately.) See esp_ptr_internal().
#define SOC_MEM_INTERNAL_LOW 0x3FCA0000
#define SOC_MEM_INTERNAL_HIGH 0x3FCE0000

#define SOC_MAX_CONTIGUOUS_RAM_SIZE (SOC_IRAM_HIGH - SOC_IRAM_LOW) ///< Largest span of contiguous memory (DRAM or IRAM) in the address space

// Region of address space that holds peripherals
#define SOC_PERIPHERAL_LOW 0x60000000
#define SOC_PERIPHERAL_HIGH 0x60100000

// Debug region, not used by software
#define SOC_DEBUG_LOW 0x20000000
#define SOC_DEBUG_HIGH 0x28000000

// Start (highest address) of ROM boot stack, only relevant during early boot
#define SOC_ROM_STACK_START 0x3fcdeb70
#define SOC_ROM_STACK_SIZE 0x2000
