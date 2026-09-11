/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>

#include "reg_base.h"

/*
 * Subset of ESP-IDF components/soc/esp32h21/register/soc/pmu_reg.h, limited to
 * what the DC-DC converter bring-up in clock.c needs.
 */

/* Conduction-mode control. CCM needs CCM_SW_EN set and CCM_ENB cleared. */
#define PMU_HP_ACTIVE_BIAS_REG     (DR_REG_PMU_BASE + 0x18)
#define PMU_HP_ACTIVE_DCDC_CCM_ENB BIT(9)

#define PMU_DCM_CTRL_REG           (DR_REG_PMU_BASE + 0x1b8)
#define PMU_DCDC_CCM_SW_EN         BIT(27)

/* High power regulator drive strength. */
#define PMU_HP_ACTIVE_HP_REGULATOR1_REG    (DR_REG_PMU_BASE + 0x2c)
#define PMU_HP_ACTIVE_HP_REGULATOR_DRV_B   0x00FFFFFFU
#define PMU_HP_ACTIVE_HP_REGULATOR_DRV_B_V 0x00FFFFFFU
#define PMU_HP_ACTIVE_HP_REGULATOR_DRV_B_S 8

/* Bit 25 controls the RF PLL, which the stub has no use for. */
#define PMU_DATE_REG               (DR_REG_PMU_BASE + 0x3fc)
#define PMU_RF_PLL_EN              BIT(25)
