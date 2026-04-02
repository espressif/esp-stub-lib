/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

__attribute__((weak)) void stub_target_clock_init(void)
{
}

__attribute__((weak)) uint32_t stub_target_get_cpu_freq(void)
{
    return 0;
}

__attribute__((weak)) uint32_t stub_target_get_apb_freq(void)
{
    return 0;
}

__attribute__((weak)) void stub_target_clock_disable_watchdogs(void)
{
}
