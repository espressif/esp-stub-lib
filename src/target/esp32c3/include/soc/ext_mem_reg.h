/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>

#include "reg_base.h"

#define EXTMEM_ICACHE_CTRL_REG              (DR_REG_EXTMEM_BASE + 0x000)
#define EXTMEM_ICACHE_ENABLE                BIT(0)

#define EXTMEM_ICACHE_CTRL1_REG             (DR_REG_EXTMEM_BASE + 0x004)
#define EXTMEM_ICACHE_SHUT_IBUS             BIT(0)
#define EXTMEM_ICACHE_SHUT_DBUS             BIT(1)
#define EXTMEM_ICACHE_BUS_MASK              0x3U

#define EXTMEM_ICACHE_AUTOLOAD_CTRL_REG     (DR_REG_EXTMEM_BASE + 0x040)
#define EXTMEM_ICACHE_AUTOLOAD_ENA          BIT(2)
