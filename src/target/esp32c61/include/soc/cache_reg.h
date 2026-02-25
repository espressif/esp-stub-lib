/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>

#include "reg_base.h"

#define CACHE_L1_CACHE_CTRL_REG             (DR_REG_CACHE_BASE + 0x004)
#define CACHE_L1_CACHE_SHUT_BUS0            BIT(0)   /* IBUS0 */
#define CACHE_L1_CACHE_SHUT_BUS1            BIT(1)   /* DBUS0 */
#define CACHE_L1_CACHE_BUS_MASK             0x3U

#define CACHE_L1_CACHE_AUTOLOAD_CTRL_REG    (DR_REG_CACHE_BASE + 0x130)
#define CACHE_L1_CACHE_AUTOLOAD_ENA         BIT(0)
