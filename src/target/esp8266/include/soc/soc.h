/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

/* Overall memory map */

/* Mapped flash */
#define SOC_IROM_LOW 0x40200000
#define SOC_IROM_HIGH 0x40300000
#define SOC_DROM_LOW 0x40200000
#define SOC_DROM_HIGH 0x40300000

#define SOC_IRAM_LOW 0x40100000
#define SOC_IRAM_HIGH 0x40108000
#define SOC_DRAM_LOW 0x3FFE8000
#define SOC_DRAM_HIGH 0x40000000

#define SOC_IROM_MASK_LOW 0x40000000
#define SOC_IROM_MASK_HIGH 0x40010000

#define SOC_RTC_DATA_LOW 0x60001200
#define SOC_RTC_DATA_HIGH 0x60001400
