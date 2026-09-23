/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * Register definitions for CPU_PLL (CPLL) on the internal regi2c bus.
 * See ESP-IDF components/soc/esp32p4/include/soc/regi2c_cpll.h.
 */

#pragma once

#define I2C_CPLL           0x67
#define I2C_CPLL_HOSTID    0

#define I2C_CPLL_OC_REF_DIV        2
#define I2C_CPLL_OC_REF_DIV_MSB    3
#define I2C_CPLL_OC_REF_DIV_LSB    0

#define I2C_CPLL_OC_DCHGP        2
#define I2C_CPLL_OC_DCHGP_MSB    6
#define I2C_CPLL_OC_DCHGP_LSB    4

#define I2C_CPLL_OC_ENB_FCAL        2
#define I2C_CPLL_OC_ENB_FCAL_MSB    7
#define I2C_CPLL_OC_ENB_FCAL_LSB    7

#define I2C_CPLL_OC_DIV_7_0        3
#define I2C_CPLL_OC_DIV_7_0_MSB    7
#define I2C_CPLL_OC_DIV_7_0_LSB    0

#define I2C_CPLL_OC_DCUR        6
#define I2C_CPLL_OC_DCUR_MSB    2
#define I2C_CPLL_OC_DCUR_LSB    0

#define I2C_CPLL_OC_DHREF_SEL        6
#define I2C_CPLL_OC_DHREF_SEL_MSB    5
#define I2C_CPLL_OC_DHREF_SEL_LSB    4

#define I2C_CPLL_OC_DLREF_SEL        6
#define I2C_CPLL_OC_DLREF_SEL_MSB    7
#define I2C_CPLL_OC_DLREF_SEL_LSB    6
