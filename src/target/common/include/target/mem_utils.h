/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdbool.h>

bool stub_target_mem_is_rtc_fast_supported(void);
bool stub_target_mem_is_rtc_slow_supported(void);
bool stub_target_mem_is_tcm_supported(void);

bool stub_target_mem_is_drom(const void *p);
bool stub_target_mem_is_irom(const void *p);

bool stub_target_mem_is_dram(const void *p);
bool stub_target_mem_is_iram(const void *p);
bool stub_target_mem_is_rtc_dram_fast(const void *p);
bool stub_target_mem_is_rtc_iram_fast(const void *p);
bool stub_target_mem_is_rtc_slow(const void *p);
bool stub_target_mem_is_tcm(const void *p);
