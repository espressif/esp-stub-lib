/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>
#include "soc/reg_base.h"

#define CACHE_L1_ICACHE_CTRL_REG        (DR_REG_CACHE_BASE + 0x0)
#define CACHE_L1_ICACHE_SHUT_IBUS0      BIT(0)
#define CACHE_L1_ICACHE_SHUT_IBUS1      BIT(1)

#define CACHE_L1_DCACHE_CTRL_REG        (DR_REG_CACHE_BASE + 0x4)
#define CACHE_L1_DCACHE_SHUT_DBUS0      BIT(0)
#define CACHE_L1_DCACHE_SHUT_DBUS1      BIT(1)

#define CACHE_SYNC_CTRL_REG             (DR_REG_CACHE_BASE + 0x98)
#define CACHE_WRITEBACK_ENA             BIT(2)
#define CACHE_SYNC_DONE                 BIT(4)

#define CACHE_SYNC_MAP_REG              (DR_REG_CACHE_BASE + 0x9c)
#define CACHE_SYNC_ADDR_REG             (DR_REG_CACHE_BASE + 0xa0)
#define CACHE_SYNC_SIZE_REG             (DR_REG_CACHE_BASE + 0xa4)
