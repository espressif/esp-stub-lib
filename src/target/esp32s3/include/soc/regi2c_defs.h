/*
 * SPDX-FileCopyrightText: 2015-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>

#define I2C_MST_ANA_CONF0_REG           0x6000E040
#define I2C_MST_BBPLL_STOP_FORCE_HIGH   BIT(2)
#define I2C_MST_BBPLL_STOP_FORCE_LOW    BIT(3)
#define I2C_MST_BBPLL_CAL_DONE          BIT(24)
