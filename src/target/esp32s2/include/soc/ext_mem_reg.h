/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>

#include "reg_base.h"

/* DCache control registers */
#define EXTMEM_PRO_DCACHE_CTRL_REG          (DR_REG_EXTMEM_BASE + 0x000)
#define EXTMEM_PRO_DCACHE_ENABLE            BIT(0)
#define EXTMEM_PRO_DCACHE_AUTOLOAD_ENA      BIT(18)

#define EXTMEM_PRO_DCACHE_CTRL1_REG         (DR_REG_EXTMEM_BASE + 0x004)
#define EXTMEM_PRO_DCACHE_MASK_BUS0         BIT(0)   /* DRAM0 */
#define EXTMEM_PRO_DCACHE_MASK_BUS1         BIT(1)   /* DRAM1 */
#define EXTMEM_PRO_DCACHE_MASK_BUS2         BIT(2)   /* DPORT */
#define EXTMEM_PRO_DCACHE_BUS_MASK          0x7U

/* ICache control registers */
#define EXTMEM_PRO_ICACHE_CTRL_REG          (DR_REG_EXTMEM_BASE + 0x040)
#define EXTMEM_PRO_ICACHE_ENABLE            BIT(0)
#define EXTMEM_PRO_ICACHE_AUTOLOAD_ENA      BIT(18)

#define EXTMEM_PRO_ICACHE_CTRL1_REG         (DR_REG_EXTMEM_BASE + 0x044)
#define EXTMEM_PRO_ICACHE_MASK_BUS0         BIT(0)   /* IRAM0 */
#define EXTMEM_PRO_ICACHE_MASK_BUS1         BIT(1)   /* IRAM1 */
#define EXTMEM_PRO_ICACHE_MASK_BUS2         BIT(2)   /* DROM0 */
#define EXTMEM_PRO_ICACHE_BUS_MASK          0x7U

#define EXTMEM_PRO_ICACHE_MASK_DROM0        EXTMEM_PRO_ICACHE_MASK_BUS2
