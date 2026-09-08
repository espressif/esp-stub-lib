/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

/**
 * @file regi2c_dcdc.h
 * @brief Register definitions of the DC-DC converter that supplies the core
 *        rail, addressed over the analog I2C bus.
 */

#pragma once

#define I2C_DCDC        0x6D
#define I2C_DCDC_HOSTID 0

#define I2C_DCDC_XPD_TRX                1
#define I2C_DCDC_XPD_TRX_MSB            7
#define I2C_DCDC_XPD_TRX_LSB            7

/* Output voltage setting in continuous conduction mode. */
#define I2C_DCDC_CCM_DREG0              7
#define I2C_DCDC_CCM_DREG0_MSB          4
#define I2C_DCDC_CCM_DREG0_LSB          0

/* Peak current limit in continuous conduction mode. */
#define I2C_DCDC_CCM_PCUR_LIMIT0        7
#define I2C_DCDC_CCM_PCUR_LIMIT0_MSB    7
#define I2C_DCDC_CCM_PCUR_LIMIT0_LSB    5

/* Output voltage setting in voltage control mode. */
#define I2C_DCDC_VCM_DREG0              10
#define I2C_DCDC_VCM_DREG0_MSB          4
#define I2C_DCDC_VCM_DREG0_LSB          0

/* Peak current limit in voltage control mode. */
#define I2C_DCDC_VCM_PCUR_LIMIT0        10
#define I2C_DCDC_VCM_PCUR_LIMIT0_MSB    7
#define I2C_DCDC_VCM_PCUR_LIMIT0_LSB    5
