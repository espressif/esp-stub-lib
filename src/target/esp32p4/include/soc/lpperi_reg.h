/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * Minimal LPPERI defs for analog I2C master clock gating (CPLL regi2c).
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>
#include "reg_base.h"

/** LPPERI_CLK_EN_REG register */
#define LPPERI_CLK_EN_REG (DR_REG_LPPERI_BASE + 0x0)
/** LPPERI_CK_EN_LP_I2CMST : R/W; bitpos: [27]; default: 1 */
#define LPPERI_CK_EN_LP_I2CMST    (BIT(27))
#define LPPERI_CK_EN_LP_I2CMST_M  (LPPERI_CK_EN_LP_I2CMST_V << LPPERI_CK_EN_LP_I2CMST_S)
#define LPPERI_CK_EN_LP_I2CMST_V  0x00000001U
#define LPPERI_CK_EN_LP_I2CMST_S  27
