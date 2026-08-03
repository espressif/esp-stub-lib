/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <soc/reg_base.h>

#define PMU_HP_ACTIVE_HP_REGULATOR0_REG    (DR_REG_PMU_BASE + 0x34U)

#define PMU_HP_ACTIVE_HP_REGULATOR_DBIAS_V 0x1FU
#define PMU_HP_ACTIVE_HP_REGULATOR_DBIAS_S 27
#define PMU_HP_ACTIVE_HP_REGULATOR_DBIAS_M (PMU_HP_ACTIVE_HP_REGULATOR_DBIAS_V << PMU_HP_ACTIVE_HP_REGULATOR_DBIAS_S)
