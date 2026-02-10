/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

/* Overall memory map */
#define SOC_IROM_LOW 0x40000000
#define SOC_IROM_HIGH 0x44000000
#define SOC_DROM_LOW 0x40000000
#define SOC_DROM_HIGH 0x44000000

#define SOC_EXTRAM_LOW 0x48000000
#define SOC_EXTRAM_HIGH 0x4c000000
#define SOC_EXTRAM_SIZE (SOC_EXTRAM_HIGH - SOC_EXTRAM_LOW)

#define SOC_IROM_MASK_LOW 0x4fc00000
#define SOC_IROM_MASK_HIGH 0x4fc20000
#define SOC_DROM_MASK_LOW 0x4fc00000
#define SOC_DROM_MASK_HIGH 0x4fc20000
#define SOC_TCM_LOW 0x30100000
#define SOC_TCM_HIGH 0x30102000
#define SOC_IRAM_LOW 0x4ff00000
#define SOC_IRAM_HIGH 0x4ffc0000
#define SOC_DRAM_LOW 0x4ff00000
#define SOC_DRAM_HIGH 0x4ffc0000
#define SOC_RTC_IRAM_LOW 0x50108000 // ESP32-P4 only has 32k LP memory
#define SOC_RTC_IRAM_HIGH 0x50110000
#define SOC_RTC_DRAM_LOW 0x50108000
#define SOC_RTC_DRAM_HIGH 0x50110000
#define SOC_RTC_DATA_LOW 0x50108000
#define SOC_RTC_DATA_HIGH 0x50110000

#define SOC_LP_ROM_LOW 0x50100000
#define SOC_LP_ROM_HIGH 0x50104000

#define SOC_LP_RAM_LOW 0x50108000
#define SOC_LP_RAM_HIGH 0x50110000

//First and last words of the D/IRAM region, for both the DRAM address as well as the IRAM alias.
#define SOC_DIRAM_IRAM_LOW 0x4ff00000
#define SOC_DIRAM_IRAM_HIGH 0x4ffc0000
#define SOC_DIRAM_DRAM_LOW 0x4ff00000
#define SOC_DIRAM_DRAM_HIGH 0x4ffc0000
#define SOC_DIRAM_ROM_RESERVE_HIGH 0x4ff40000

#define MAP_DRAM_TO_IRAM(addr) (addr)
#define MAP_IRAM_TO_DRAM(addr) (addr)

// Region of memory accessible via DMA. See esp_ptr_dma_capable().
#define SOC_DMA_LOW 0x4ff00000
#define SOC_DMA_HIGH 0x4ffc0000

// Region of RAM that is byte-accessible. See esp_ptr_byte_accessible().
#define SOC_BYTE_ACCESSIBLE_LOW 0x4ff00000
#define SOC_BYTE_ACCESSIBLE_HIGH 0x4ffc0000

//Region of memory that is internal, as in on the same silicon die as the ESP32 CPUs
//(excluding RTC data region, that's checked separately.) See esp_ptr_internal().
#define SOC_MEM_INTERNAL_LOW 0x4ff00000
#define SOC_MEM_INTERNAL_HIGH 0x4ffc0000
#define SOC_MEM_INTERNAL_LOW1 0x4ff00000
#define SOC_MEM_INTERNAL_HIGH1 0x4ffc0000

#define SOC_MAX_CONTIGUOUS_RAM_SIZE (SOC_EXTRAM_HIGH - SOC_EXTRAM_LOW) ///< Largest span of contiguous memory (DRAM or IRAM) in the address space

#define CPU_PERIPH_LOW 0x3ff00000
#define CPU_PERIPH_HIGH 0x3ff20000

// Region of address space that holds peripherals, HP APB peripherals
#define SOC_PERIPHERAL_LOW 0x50000000
#define SOC_PERIPHERAL_HIGH 0x50100000

#define SOC_LP_PERIPH_LOW 0x50110000
#define SOC_LP_PERIPH_HIGH 0x50130000

// CPU sub-system region, contains interrupt config registers
#define SOC_CPU_SUBSYSTEM_LOW 0x20000000
#define SOC_CPU_SUBSYSTEM_HIGH 0x30000000

// Start (highest address) of ROM boot stack, only relevant during early boot
#define SOC_ROM_STACK_START_REV2 0x4ffbcfc0
#define SOC_ROM_STACK_START 0x4ff3cfc0
#define SOC_ROM_STACK_SIZE 0x2000

#define LP_ROM_DRAM_START 0x5010fa80 // Value taken from ROM elf, includes LP ROM stack
#define LP_RAM_END 0x50110000
#define LP_ROM_DRAM_SIZE (LP_RAM_END - LP_ROM_DRAM_START)
