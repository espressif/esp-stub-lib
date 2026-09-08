/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * Subset of PMU register definitions needed to bring up the DC-DC converter
 * that supplies the core rail.
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>
#include "reg_base.h"

/** PMU_HP_ACTIVE_BIAS_REG register */
#define PMU_HP_ACTIVE_BIAS_REG (DR_REG_PMU_BASE + 0x18)
/** PMU_HP_ACTIVE_DCDC_CCM_ENB : R/W; bitpos: [9] */
#define PMU_HP_ACTIVE_DCDC_CCM_ENB   (BIT(9))
#define PMU_HP_ACTIVE_DCDC_CCM_ENB_M (PMU_HP_ACTIVE_DCDC_CCM_ENB_V << PMU_HP_ACTIVE_DCDC_CCM_ENB_S)
#define PMU_HP_ACTIVE_DCDC_CCM_ENB_V 0x00000001U
#define PMU_HP_ACTIVE_DCDC_CCM_ENB_S 9

/** PMU_HP_ACTIVE_HP_REGULATOR1_REG register */
#define PMU_HP_ACTIVE_HP_REGULATOR1_REG (DR_REG_PMU_BASE + 0x2c)
/** PMU_HP_ACTIVE_HP_REGULATOR_DRV_B : R/W; bitpos: [31:8] */
#define PMU_HP_ACTIVE_HP_REGULATOR_DRV_B   0x00FFFFFFU
#define PMU_HP_ACTIVE_HP_REGULATOR_DRV_B_M (PMU_HP_ACTIVE_HP_REGULATOR_DRV_B_V << PMU_HP_ACTIVE_HP_REGULATOR_DRV_B_S)
#define PMU_HP_ACTIVE_HP_REGULATOR_DRV_B_V 0x00FFFFFFU
#define PMU_HP_ACTIVE_HP_REGULATOR_DRV_B_S 8

/** PMU_DCM_CTRL_REG register */
#define PMU_DCM_CTRL_REG (DR_REG_PMU_BASE + 0x1b8)
/** PMU_DCDC_CCM_SW_EN : R/W; bitpos: [27] */
#define PMU_DCDC_CCM_SW_EN   (BIT(27))
#define PMU_DCDC_CCM_SW_EN_M (PMU_DCDC_CCM_SW_EN_V << PMU_DCDC_CCM_SW_EN_S)
#define PMU_DCDC_CCM_SW_EN_V 0x00000001U
#define PMU_DCDC_CCM_SW_EN_S 27
