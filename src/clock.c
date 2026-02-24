/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <rom_wrappers.h>

#include <target/clock.h>

void stub_lib_clock_init(void)
{
    stub_target_clock_init();
    stub_lib_delay_us(50); // Wait for the clocks to stabilize
}

void stub_lib_clock_disable_watchdogs(void)
{
    stub_target_clock_disable_watchdogs();
}
