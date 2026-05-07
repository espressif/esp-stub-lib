/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>

#include "reg_base.h"

#define LP_AON_SRAM_USAGE_CONF_REG (DR_REG_LP_AON_BASE + 0x98)

/** LP_AON_DCACHE_USAGE : R/W; bitpos: [0]; default: 0;
 *  hp system memory is split to 7 layers(Layer0 ~ Layer6) in total, this field is
 *  used to control the first layer(Layer0) usage. 0: cpu use hp-memory. 1: dcache use
 *  hp-mmory.
 *  By default, dcache is closed, and typically users can enable dcache after
 *  boot-loader, but before user's BIN start running.
 */
#define LP_AON_DCACHE_USAGE    (BIT(0))
#define LP_AON_DCACHE_USAGE_M  (LP_AON_DCACHE_USAGE_V << LP_AON_DCACHE_USAGE_S)
#define LP_AON_DCACHE_USAGE_V  0x00000001U
#define LP_AON_DCACHE_USAGE_S  0

/** LP_AON_ICACHE1_USAGE : R/W; bitpos: [1]; default: 1;
 *  hp system memory is split to 7 layers(Layer0 ~ Layer6) in total, this field is
 *  used to control the last layer(Layer6) usage. 0: cpu use hp-memory. 1: icache1 use
 *  hp-mmory.
 *  By default, icache1 is not disabled, and the last layer memory belongs to icache1.
 *  Typically users can set this bit to 0 to disable icache1 in boot-loader.
 */
#define LP_AON_ICACHE1_USAGE    (BIT(1))
#define LP_AON_ICACHE1_USAGE_M  (LP_AON_ICACHE1_USAGE_V << LP_AON_ICACHE1_USAGE_S)
#define LP_AON_ICACHE1_USAGE_V  0x00000001U
#define LP_AON_ICACHE1_USAGE_S  1
