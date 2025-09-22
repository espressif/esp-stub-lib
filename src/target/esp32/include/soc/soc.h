/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

/* Overall memory map */
#define SOC_DROM_LOW 0x3F400000
#define SOC_DROM_HIGH 0x3F800000
#define SOC_DRAM_LOW 0x3FFAE000
#define SOC_DRAM_HIGH 0x40000000
#define SOC_IROM_LOW 0x400D0000
#define SOC_IROM_HIGH 0x40400000
#define SOC_IROM_MASK_LOW 0x40000000
#define SOC_IROM_MASK_HIGH 0x40070000
#define SOC_CACHE_PRO_LOW 0x40070000
#define SOC_CACHE_PRO_HIGH 0x40078000
#define SOC_CACHE_APP_LOW 0x40078000
#define SOC_CACHE_APP_HIGH 0x40080000
#define SOC_IRAM_LOW 0x40080000
#define SOC_IRAM_HIGH 0x400AA000
#define SOC_RTC_IRAM_LOW 0x400C0000
#define SOC_RTC_IRAM_HIGH 0x400C2000
#define SOC_RTC_DRAM_LOW 0x3FF80000
#define SOC_RTC_DRAM_HIGH 0x3FF82000
#define SOC_RTC_DATA_LOW 0x50000000
#define SOC_RTC_DATA_HIGH 0x50002000
#define SOC_EXTRAM_DATA_LOW 0x3F800000
#define SOC_EXTRAM_DATA_HIGH 0x3FC00000

#define SOC_EXTRAM_DATA_SIZE (SOC_EXTRAM_DATA_HIGH - SOC_EXTRAM_DATA_LOW)

//First and last words of the D/IRAM region, for both the DRAM address as well as the IRAM alias.
#define SOC_DIRAM_IRAM_LOW 0x400A0000
#define SOC_DIRAM_IRAM_HIGH 0x400C0000
#define SOC_DIRAM_DRAM_LOW 0x3FFE0000
#define SOC_DIRAM_DRAM_HIGH 0x40000000
// Byte order of D/IRAM regions is reversed between accessing as DRAM or IRAM
#define SOC_DIRAM_INVERTED 1

// Region of memory accessible via DMA. See esp_ptr_dma_capable().
#define SOC_DMA_LOW 0x3FFAE000
#define SOC_DMA_HIGH 0x40000000

// Region of memory that is byte-accessible. See esp_ptr_byte_accessible().
#define SOC_BYTE_ACCESSIBLE_LOW 0x3FF90000
#define SOC_BYTE_ACCESSIBLE_HIGH 0x40000000

//Region of memory that is internal, as in on the same silicon die as the ESP32 CPUs
//(excluding RTC data region, that's checked separately.) See esp_ptr_internal().
#define SOC_MEM_INTERNAL_LOW 0x3FF90000
#define SOC_MEM_INTERNAL_HIGH 0x400C2000

// Start (highest address) of ROM boot stack, only relevant during early boot
#define SOC_ROM_STACK_START 0x3ffe3f20
