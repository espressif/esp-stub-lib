/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

bool stub_target_mem_is_irom(uintptr_t addr);
bool stub_target_mem_is_drom(uintptr_t addr);
bool stub_target_mem_is_iram(uintptr_t addr);
bool stub_target_mem_is_dram(uintptr_t addr);
bool stub_target_mem_is_rtc_iram_fast(uintptr_t addr);
bool stub_target_mem_is_rtc_dram_fast(uintptr_t addr);
bool stub_target_mem_is_rtc_slow(uintptr_t addr);
bool stub_target_mem_is_tcm(uintptr_t addr);
