/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>
#include <target/flash.h>

#define SOC_RTC_FAST_MEM_SUPPORTED 1
#define SOC_RTC_SLOW_MEM_SUPPORTED 1
#define SOC_MEM_TCM_SUPPORTED 0

#define SOC_DROM_LOW 0x3C000000
#define SOC_DROM_HIGH 0x3E000000
#define SOC_IROM_LOW 0x42000000
#define SOC_IROM_HIGH 0x44000000

#define SOC_DRAM_LOW 0x3FC88000
#define SOC_DRAM_HIGH 0x3FD00000
#define SOC_IRAM_LOW 0x40370000
#define SOC_IRAM_HIGH 0x403E0000

#define SOC_RTC_DRAM_LOW 0x600FE000
#define SOC_RTC_DRAM_HIGH 0x60100000
#define SOC_RTC_IRAM_LOW 0x600FE000
#define SOC_RTC_IRAM_HIGH 0x60100000
#define SOC_RTC_DATA_LOW 0x50000000
#define SOC_RTC_DATA_HIGH 0x50002000

bool stub_target_mem_is_rtc_fast_supported(void)
{
    return SOC_RTC_FAST_MEM_SUPPORTED;
}

bool stub_target_mem_is_rtc_slow_supported(void)
{
    return SOC_RTC_SLOW_MEM_SUPPORTED;
}

bool stub_target_mem_is_tcm_supported(void)
{
    return SOC_MEM_TCM_SUPPORTED;
}

bool stub_target_mem_is_drom(const void *p)
{
    return (uintptr_t)p >= SOC_DROM_LOW && (uintptr_t)p < SOC_DROM_HIGH;
}

bool stub_target_mem_is_irom(const void *p)
{
    return (uintptr_t)p >= SOC_IROM_LOW && (uintptr_t)p < SOC_IROM_HIGH;
}

bool stub_target_mem_is_dram(const void *p)
{
    return (uintptr_t)p >= SOC_DRAM_LOW && (uintptr_t)p < SOC_DRAM_HIGH;
}

bool stub_target_mem_is_iram(const void *p)
{
    return (uintptr_t)p >= SOC_IRAM_LOW && (uintptr_t)p < SOC_IRAM_HIGH;
}

bool stub_target_mem_is_rtc_dram_fast(const void *p)
{
    return (uintptr_t)p >= SOC_RTC_DRAM_LOW && (uintptr_t)p < SOC_RTC_DRAM_HIGH;
}

bool stub_target_mem_is_rtc_iram_fast(const void *p)
{
    return (uintptr_t)p >= SOC_RTC_IRAM_LOW && (uintptr_t)p < SOC_RTC_IRAM_HIGH;
}

bool stub_target_mem_is_rtc_slow(const void *p)
{
    return (uintptr_t)p >= SOC_RTC_DATA_LOW && (uintptr_t)p < SOC_RTC_DATA_HIGH;
}

bool stub_target_mem_is_tcm(const void *p)
{
    (void)p;
    return false;
}
