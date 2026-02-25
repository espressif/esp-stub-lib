/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>

#include "reg_base.h"

#define EXTMEM_L1_CACHE_CTRL_REG           (DR_REG_CACHE_BASE + 0x4)
#define EXTMEM_L1_CACHE_SHUT_DBUS          BIT(1)
#define EXTMEM_L1_CACHE_SHUT_IBUS          BIT(0)

#define EXTMEM_L1_CACHE_AUTOLOAD_CTRL_REG  (DR_REG_CACHE_BASE + 0x130)
#define EXTMEM_L1_CACHE_AUTOLOAD_ENA       BIT(0)
