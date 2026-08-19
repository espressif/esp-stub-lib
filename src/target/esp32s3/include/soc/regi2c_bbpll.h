/*
 * SPDX-FileCopyrightText: 2015-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#define I2C_BBPLL                      0x66
#define I2C_BBPLL_HOSTID               1

#define I2C_BBPLL_OC_REF_DIV           2
#define I2C_BBPLL_OC_DCHGP_LSB         4
#define I2C_BBPLL_OC_DIV_7_0           3
#define I2C_BBPLL_MODE_HF              4

#define I2C_BBPLL_OC_DR1               5
#define I2C_BBPLL_OC_DR1_MSB           2
#define I2C_BBPLL_OC_DR1_LSB           0

#define I2C_BBPLL_OC_DR3               5
#define I2C_BBPLL_OC_DR3_MSB           6
#define I2C_BBPLL_OC_DR3_LSB           4

#define I2C_BBPLL_OC_DCUR              6
#define I2C_BBPLL_OC_DHREF_SEL_LSB     4
#define I2C_BBPLL_OC_DLREF_SEL_LSB     6

#define I2C_BBPLL_OC_VCO_DBIAS         9
#define I2C_BBPLL_OC_VCO_DBIAS_MSB     1
#define I2C_BBPLL_OC_VCO_DBIAS_LSB     0
