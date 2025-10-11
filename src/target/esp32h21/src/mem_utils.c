/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <stdbool.h>
#include <stdint.h>

#include <target/mem_utils.h>

#include <soc/soc.h>

bool stub_target_mem_is_rtc_iram_fast(uintptr_t addr)
{
    return addr >= SOC_RTC_IRAM_LOW && addr < SOC_RTC_IRAM_HIGH;
}

bool stub_target_mem_is_rtc_dram_fast(uintptr_t addr)
{
    return addr >= SOC_RTC_DRAM_LOW && addr < SOC_RTC_DRAM_HIGH;
}

bool stub_target_mem_is_rtc_slow(uintptr_t addr)
{
    return addr >= SOC_RTC_DATA_LOW && addr < SOC_RTC_DATA_HIGH;
}
