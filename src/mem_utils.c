/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*/
#include <stdbool.h>
#include <stdint.h>

#include <mem_utils.h>

#include <soc/soc.h>

bool stub_lib_mem_is_irom(const void *p)
{
#if defined(SOC_IROM_LOW) && defined(SOC_IROM_HIGH)
    return (uintptr_t)p >= SOC_IROM_LOW && (uintptr_t)p < SOC_IROM_HIGH;
#else
    return false;
#endif
}

bool stub_lib_mem_is_drom(const void *p)
{
#if defined(SOC_DROM_LOW) && defined(SOC_DROM_HIGH)
    return (uintptr_t)p >= SOC_DROM_LOW && (uintptr_t)p < SOC_DROM_HIGH;
#else
    return false;
#endif
}

bool stub_lib_mem_is_iram(const void *p)
{
#if defined(SOC_IRAM_LOW) && defined(SOC_IRAM_HIGH)
    return (uintptr_t)p >= SOC_IRAM_LOW && (uintptr_t)p < SOC_IRAM_HIGH;
#else
    return false;
#endif
}

bool stub_lib_mem_is_dram(const void *p)
{
#if defined(SOC_DRAM_LOW) && defined(SOC_DRAM_HIGH)
    return (uintptr_t)p >= SOC_DRAM_LOW && (uintptr_t)p < SOC_DRAM_HIGH;
#else
    return false;
#endif
}

bool stub_lib_mem_is_rtc_iram_fast(const void *p)
{
#if defined(SOC_RTC_IRAM_LOW) && defined(SOC_RTC_IRAM_HIGH)
    return (uintptr_t)p >= SOC_RTC_IRAM_LOW && (uintptr_t)p < SOC_RTC_IRAM_HIGH;
#else
    (void)p;
    return false;
#endif
}

bool stub_lib_mem_is_rtc_dram_fast(const void *p)
{
#if defined(SOC_RTC_DRAM_LOW) && defined(SOC_RTC_DRAM_HIGH)
    return (uintptr_t)p >= SOC_RTC_DRAM_LOW && (uintptr_t)p < SOC_RTC_DRAM_HIGH;
#else
    (void)p;
    return false;
#endif

}

bool stub_lib_mem_is_rtc_slow(const void *p)
{
#if defined(SOC_RTC_DATA_LOW) && defined(SOC_RTC_DATA_HIGH)
    return (uintptr_t)p >= SOC_RTC_DATA_LOW && (uintptr_t)p < SOC_RTC_DATA_HIGH;
#else
    (void)p;
    return false;
#endif
}

bool stub_lib_mem_is_tcm(const void *p)
{
#if defined(SOC_TCM_LOW) && defined(SOC_TCM_HIGH)
    return ((uintptr_t)p >= SOC_TCM_LOW && (uintptr_t)p < SOC_TCM_HIGH);
#else
    (void)p;
    return false;
#endif
}
