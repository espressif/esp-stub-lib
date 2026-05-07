/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * Subset of PMU register definitions for stub flash download power sequencing.
 * See ESP-IDF components/soc/esp32p4/register/hw_ver3/soc/pmu_reg.h.
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>
#include "reg_base.h"

/** PMU_INT_RAW_REG register */
#define PMU_INT_RAW_REG (DR_REG_PMU_BASE + 0x164)
/** PMU_0P1A_CNT_TARGET0_REACH_0_HP_INT_RAW : R/WTC/SS; bitpos: [14] */
#define PMU_0P1A_CNT_TARGET0_REACH_0_HP_INT_RAW    (BIT(14))
#define PMU_0P1A_CNT_TARGET0_REACH_0_HP_INT_RAW_M  (PMU_0P1A_CNT_TARGET0_REACH_0_HP_INT_RAW_V << PMU_0P1A_CNT_TARGET0_REACH_0_HP_INT_RAW_S)
#define PMU_0P1A_CNT_TARGET0_REACH_0_HP_INT_RAW_V  0x00000001U
#define PMU_0P1A_CNT_TARGET0_REACH_0_HP_INT_RAW_S  14

/** PMU_HP_INT_CLR_REG register */
#define PMU_HP_INT_CLR_REG (DR_REG_PMU_BASE + 0x170)
/** PMU_0P1A_CNT_TARGET0_REACH_0_HP_INT_CLR : WT; bitpos: [14] */
#define PMU_0P1A_CNT_TARGET0_REACH_0_HP_INT_CLR    (BIT(14))
#define PMU_0P1A_CNT_TARGET0_REACH_0_HP_INT_CLR_M  (PMU_0P1A_CNT_TARGET0_REACH_0_HP_INT_CLR_V << PMU_0P1A_CNT_TARGET0_REACH_0_HP_INT_CLR_S)
#define PMU_0P1A_CNT_TARGET0_REACH_0_HP_INT_CLR_V  0x00000001U
#define PMU_0P1A_CNT_TARGET0_REACH_0_HP_INT_CLR_S  14

/** PMU_EXT_LDO_P0_0P1A_REG register */
#define PMU_EXT_LDO_P0_0P1A_REG (DR_REG_PMU_BASE + 0x1b8)
/** PMU_0P1A_TARGET0_0 : R/W; bitpos: [30:23] */
#define PMU_0P1A_TARGET0_0    0x000000FFU
#define PMU_0P1A_TARGET0_0_M  (PMU_0P1A_TARGET0_0_V << PMU_0P1A_TARGET0_0_S)
#define PMU_0P1A_TARGET0_0_V  0x000000FFU
#define PMU_0P1A_TARGET0_0_S  23

/** PMU_DATE_REG register */
#define PMU_DATE_REG (DR_REG_PMU_BASE + 0x3fc)
/** PMU_PMU_DATE : R/W; bitpos: [30:0] */
#define PMU_PMU_DATE    0x7FFFFFFFU
#define PMU_PMU_DATE_M  (PMU_PMU_DATE_V << PMU_PMU_DATE_S)
#define PMU_PMU_DATE_V  0x7FFFFFFFU
#define PMU_PMU_DATE_S  0
