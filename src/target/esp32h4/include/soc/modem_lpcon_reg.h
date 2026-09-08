/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * Subset of MODEM_LPCON register definitions needed to clock the analog I2C
 * master.
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>
#include "reg_base.h"

/** MODEM_LPCON_CLK_CONF_REG register */
#define MODEM_LPCON_CLK_CONF_REG (DR_REG_MODEM_LPCON_BASE + 0x18)
/** MODEM_LPCON_CLK_I2C_MST_EN : R/W; bitpos: [2] */
#define MODEM_LPCON_CLK_I2C_MST_EN   (BIT(2))
#define MODEM_LPCON_CLK_I2C_MST_EN_M (MODEM_LPCON_CLK_I2C_MST_EN_V << MODEM_LPCON_CLK_I2C_MST_EN_S)
#define MODEM_LPCON_CLK_I2C_MST_EN_V 0x00000001U
#define MODEM_LPCON_CLK_I2C_MST_EN_S 2
