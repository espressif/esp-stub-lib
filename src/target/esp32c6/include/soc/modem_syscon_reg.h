/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * Subset of MODEM_SYSCON register definitions needed to ungate the modem APB
 * clock domain.
 */

#pragma once

#include "reg_base.h"

/** MODEM_SYSCON_CLK_CONF_POWER_ST_REG register */
#define MODEM_SYSCON_CLK_CONF_POWER_ST_REG (DR_REG_MODEM_SYSCON_BASE + 0xc)
/** MODEM_SYSCON_CLK_MODEM_APB_ST_MAP : R/W; bitpos: [31:28] */
#define MODEM_SYSCON_CLK_MODEM_APB_ST_MAP    0x0000000FU
#define MODEM_SYSCON_CLK_MODEM_APB_ST_MAP_M  (MODEM_SYSCON_CLK_MODEM_APB_ST_MAP_V << MODEM_SYSCON_CLK_MODEM_APB_ST_MAP_S)
#define MODEM_SYSCON_CLK_MODEM_APB_ST_MAP_V  0x0000000FU
#define MODEM_SYSCON_CLK_MODEM_APB_ST_MAP_S  28
