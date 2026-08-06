/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>

#include "reg_base.h"

/* CPU clock configuration */
#define DPORT_CPU_PER_CONF_REG      (DR_REG_DPORT_BASE + 0x03C)
#define DPORT_CPUPERIOD_SEL         0x00000003
#define DPORT_CPUPERIOD_SEL_M       ((DPORT_CPUPERIOD_SEL_V) << (DPORT_CPUPERIOD_SEL_S))
#define DPORT_CPUPERIOD_SEL_V       0x3
#define DPORT_CPUPERIOD_SEL_S       0

/* PRO CPU cache control */
#define DPORT_PRO_CACHE_CTRL_REG    (DR_REG_DPORT_BASE + 0x040)
#define DPORT_PRO_CACHE_ENABLE      BIT(3)
#define DPORT_PRO_DRAM_HL           BIT(16)

#define DPORT_PRO_CACHE_CTRL1_REG   (DR_REG_DPORT_BASE + 0x044)
#define DPORT_PRO_CACHE_MASK_DRAM1  BIT(3)

/* APP CPU cache control */
#define DPORT_APP_CACHE_CTRL_REG    (DR_REG_DPORT_BASE + 0x058)
#define DPORT_APP_CACHE_ENABLE      BIT(3)

#define DPORT_APP_CACHE_CTRL1_REG   (DR_REG_DPORT_BASE + 0x05c)

/* CTRL1 bus mask: bits [5:0] — each bit disables one bus when set */
#define DPORT_CACHE_BUS_MASK        0x3fU
#define DPORT_PRO_CACHE_MASK_DROM0  BIT(4)
#define DPORT_PRO_CACHE_MASK_IROM0  BIT(2)

/* PRO CPU flash MMU table (memory-mapped, 384 entries) */
#define DPORT_PRO_FLASH_MMU_TABLE   ((volatile uint32_t *)0x3FF10000)
