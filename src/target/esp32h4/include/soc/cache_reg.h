/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>

#include "reg_base.h"

/* ICache control: bus shut bits per core (IBUS0 = core0, IBUS1 = core1) */
#define CACHE_L1_ICACHE_CTRL_REG            (DR_REG_CACHE_BASE + 0x000)
#define CACHE_L1_ICACHE_SHUT_IBUS0          BIT(0)
#define CACHE_L1_ICACHE_SHUT_IBUS1          BIT(1)
#define CACHE_L1_ICACHE_BUS_MASK            0x3U

/* DCache control: bus shut bits per core (DBUS0 = core0, DBUS1 = core1) */
#define CACHE_L1_DCACHE_CTRL_REG            (DR_REG_CACHE_BASE + 0x004)
#define CACHE_L1_DCACHE_SHUT_DBUS0          BIT(0)
#define CACHE_L1_DCACHE_SHUT_DBUS1          BIT(1)
#define CACHE_L1_DCACHE_BUS_MASK            0x3U

#define CACHE_L1_ICACHE0_AUTOLOAD_CTRL_REG  (DR_REG_CACHE_BASE + 0x0e8)
#define CACHE_L1_ICACHE0_AUTOLOAD_ENA       BIT(0)

#define CACHE_L1_ICACHE1_AUTOLOAD_CTRL_REG  (DR_REG_CACHE_BASE + 0x0fc)
#define CACHE_L1_ICACHE1_AUTOLOAD_ENA       BIT(0)

#define CACHE_L1_DCACHE_AUTOLOAD_CTRL_REG   (DR_REG_CACHE_BASE + 0x138)
#define CACHE_L1_DCACHE_AUTOLOAD_ENA        BIT(0)
