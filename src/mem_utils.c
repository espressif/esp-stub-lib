/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <stdbool.h>
#include <stdint.h>

#include <mem_utils.h>
#include <target/mem_utils.h>

bool stub_lib_mem_is_irom(uintptr_t addr)
{
    return stub_target_mem_is_irom(addr);
}

bool stub_lib_mem_is_drom(uintptr_t addr)
{
    return stub_target_mem_is_drom(addr);
}

bool stub_lib_mem_is_iram(uintptr_t addr)
{
    return stub_target_mem_is_iram(addr);
}

bool stub_lib_mem_is_dram(uintptr_t addr)
{
    return stub_target_mem_is_dram(addr);
}

bool stub_lib_mem_is_rtc_iram_fast(uintptr_t addr)
{
    return stub_target_mem_is_rtc_iram_fast(addr);
}

bool stub_lib_mem_is_rtc_dram_fast(uintptr_t addr)
{
    return stub_target_mem_is_rtc_dram_fast(addr);
}

bool stub_lib_mem_is_rtc_slow(uintptr_t addr)
{
    return stub_target_mem_is_rtc_slow(addr);
}

bool stub_lib_mem_is_tcm(uintptr_t addr)
{
    return stub_target_mem_is_tcm(addr);
}
