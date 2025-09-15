/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*/
#include <stdbool.h>
#include <stdint.h>

#include <mem_utils.h>

#include <soc/soc.h>

bool stub_lib_mem_is_irom(uintptr_t addr)
{
#if defined(SOC_IROM_LOW) && defined(SOC_IROM_HIGH)
    return addr >= SOC_IROM_LOW && addr < SOC_IROM_HIGH;
#else
    (void)addr;
    return false;
#endif
}

bool stub_lib_mem_is_drom(uintptr_t addr)
{
#if defined(SOC_DROM_LOW) && defined(SOC_DROM_HIGH)
    return addr >= SOC_DROM_LOW && addr < SOC_DROM_HIGH;
#else
    (void)addr;
    return false;
#endif
}

bool stub_lib_mem_is_iram(uintptr_t addr)
{
#if defined(SOC_IRAM_LOW) && defined(SOC_IRAM_HIGH)
    return addr >= SOC_IRAM_LOW && addr < SOC_IRAM_HIGH;
#else
    (void)addr;
    return false;
#endif
}

bool stub_lib_mem_is_dram(uintptr_t addr)
{
#if defined(SOC_DRAM_LOW) && defined(SOC_DRAM_HIGH)
    return addr >= SOC_DRAM_LOW && addr < SOC_DRAM_HIGH;
#else
    (void)addr;
    return false;
#endif
}

bool stub_lib_mem_is_rtc_iram_fast(uintptr_t addr)
{
#if defined(SOC_RTC_IRAM_LOW) && defined(SOC_RTC_IRAM_HIGH)
    return addr >= SOC_RTC_IRAM_LOW && addr < SOC_RTC_IRAM_HIGH;
#else
    (void)addr;
    return false;
#endif
}

bool stub_lib_mem_is_rtc_dram_fast(uintptr_t addr)
{
#if defined(SOC_RTC_DRAM_LOW) && defined(SOC_RTC_DRAM_HIGH)
    return addr >= SOC_RTC_DRAM_LOW && addr < SOC_RTC_DRAM_HIGH;
#else
    (void)addr;
    return false;
#endif

}

bool stub_lib_mem_is_rtc_slow(uintptr_t addr)
{
#if defined(SOC_RTC_DATA_LOW) && defined(SOC_RTC_DATA_HIGH)
    return addr >= SOC_RTC_DATA_LOW && addr < SOC_RTC_DATA_HIGH;
#else
    (void)addr;
    return false;
#endif
}

bool stub_lib_mem_is_tcm(uintptr_t addr)
{
#if defined(SOC_TCM_LOW) && defined(SOC_TCM_HIGH)
    return (addr >= SOC_TCM_LOW && addr < SOC_TCM_HIGH);
#else
    (void)addr;
    return false;
#endif
}
