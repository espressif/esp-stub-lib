/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>

/* Analog function control register */
#define ANA_CONFIG_REG 0x6000E044
/* Clear to enable BBPLL */
#define I2C_BBPLL_M    (BIT(17))
