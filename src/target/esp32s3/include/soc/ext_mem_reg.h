/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>

#include "reg_base.h"

#define DR_REG_MMU_TABLE                        0x600C5000

/* DCache control registers */
#define EXTMEM_DCACHE_CTRL_REG              (DR_REG_EXTMEM_BASE + 0x000)
#define EXTMEM_DCACHE_ENABLE                BIT(0)

#define EXTMEM_DCACHE_CTRL1_REG             (DR_REG_EXTMEM_BASE + 0x004)
#define EXTMEM_DCACHE_SHUT_CORE0_BUS        BIT(0)
#define EXTMEM_DCACHE_SHUT_CORE1_BUS        BIT(1)
#define EXTMEM_DCACHE_BUS_MASK              0x3U

#define EXTMEM_DCACHE_AUTOLOAD_CTRL_REG     (DR_REG_EXTMEM_BASE + 0x04C)
#define EXTMEM_DCACHE_AUTOLOAD_ENA          BIT(2)

/* ICache control registers */
#define EXTMEM_ICACHE_CTRL_REG              (DR_REG_EXTMEM_BASE + 0x060)
#define EXTMEM_ICACHE_ENABLE                BIT(0)

#define EXTMEM_ICACHE_CTRL1_REG             (DR_REG_EXTMEM_BASE + 0x064)
#define EXTMEM_ICACHE_SHUT_CORE0_BUS        BIT(0)
#define EXTMEM_ICACHE_SHUT_CORE1_BUS        BIT(1)
#define EXTMEM_ICACHE_BUS_MASK              0x3U

#define EXTMEM_ICACHE_AUTOLOAD_CTRL_REG     (DR_REG_EXTMEM_BASE + 0x0A0)
#define EXTMEM_ICACHE_AUTOLOAD_ENA          BIT(2)
