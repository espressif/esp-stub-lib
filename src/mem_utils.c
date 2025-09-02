/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <stdbool.h>
#include <target/mem_utils.h>

bool stub_lib_mem_is_rtc_fast_supported(void)
{
    return stub_target_mem_is_rtc_fast_supported();
}

bool stub_lib_mem_is_rtc_slow_supported(void)
{
    return stub_target_mem_is_rtc_slow_supported();
}

bool stub_lib_mem_is_tcm_supported(void)
{
    return stub_target_mem_is_tcm_supported();
}

bool stub_lib_mem_is_drom(const void *p)
{
    return stub_target_mem_is_drom(p);
}

bool stub_lib_mem_is_irom(const void *p)
{
    return stub_target_mem_is_irom(p);
}

bool stub_lib_mem_is_dram(const void *p)
{
    return stub_target_mem_is_dram(p);
}
bool stub_lib_mem_is_iram(const void *p)
{
    return stub_target_mem_is_iram(p);
}

bool stub_lib_mem_is_rtc_dram_fast(const void *p)
{
    return stub_target_mem_is_rtc_dram_fast(p);
}

bool stub_lib_mem_is_rtc_iram_fast(const void *p)
{
    return stub_target_mem_is_rtc_iram_fast(p);
}

bool stub_lib_mem_is_rtc_slow(const void *p)
{
    return stub_target_mem_is_rtc_slow(p);
}

bool stub_lib_mem_is_tcm(const void *p)
{
    return stub_target_mem_is_tcm(p);
}
