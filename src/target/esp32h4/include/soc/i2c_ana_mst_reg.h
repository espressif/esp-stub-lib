/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * Subset of analog I2C master register definitions, used to reach the
 * analog blocks that are not memory mapped.
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>
#include "reg_base.h"

/** I2C_ANA_MST_I2C0_CTRL_REG register */
#define I2C_ANA_MST_I2C0_CTRL_REG   (DR_REG_I2C_ANA_MST_BASE + 0x0)
/** 0: I2C_ANA_MST_I2C0_CTRL_REG; 1: I2C_ANA_MST_I2C1_CTRL_REG */
#define I2C_ANA_MST_I2C_CTRL_REG(n) (I2C_ANA_MST_I2C0_CTRL_REG + (n) * 4)

/** REGI2C_RTC_BUSY : RO; bitpos: [25] */
#define REGI2C_RTC_BUSY        (BIT(25))
#define REGI2C_RTC_BUSY_M      (BIT(25))
#define REGI2C_RTC_BUSY_V      0x1
#define REGI2C_RTC_BUSY_S      25

/** REGI2C_RTC_WR_CNTL : R/W; bitpos: [24]; 0: read, 1: write */
#define REGI2C_RTC_WR_CNTL     (BIT(24))
#define REGI2C_RTC_WR_CNTL_M   (BIT(24))
#define REGI2C_RTC_WR_CNTL_V   0x1
#define REGI2C_RTC_WR_CNTL_S   24

/** REGI2C_RTC_DATA : R/W; bitpos: [23:16] */
#define REGI2C_RTC_DATA        0x000000FF
#define REGI2C_RTC_DATA_V      0xFF
#define REGI2C_RTC_DATA_S      16

/** REGI2C_RTC_ADDR : R/W; bitpos: [15:8] */
#define REGI2C_RTC_ADDR        0x000000FF
#define REGI2C_RTC_ADDR_V      0xFF
#define REGI2C_RTC_ADDR_S      8

/** REGI2C_RTC_SLAVE_ID : R/W; bitpos: [7:0] */
#define REGI2C_RTC_SLAVE_ID    0x000000FF
#define REGI2C_RTC_SLAVE_ID_V  0xFF
#define REGI2C_RTC_SLAVE_ID_S  0

/** I2C_ANA_MST_ANA_CONF1_REG register */
#define I2C_ANA_MST_ANA_CONF1_REG (DR_REG_I2C_ANA_MST_BASE + 0x1C)
#define I2C_ANA_MST_ANA_CONF1_V   0xFFFFFF
#define I2C_ANA_MST_ANA_CONF1_S   0
#define I2C_ANA_MST_ANA_CONF1_M   (I2C_ANA_MST_ANA_CONF1_V << I2C_ANA_MST_ANA_CONF1_S)
/** REGI2C_CONF1_DCDC_SEL : R/W; bitpos: [10] */
#define REGI2C_CONF1_DCDC_SEL     (BIT(10))

/** I2C_ANA_MST_ANA_CONF2_REG register */
#define I2C_ANA_MST_ANA_CONF2_REG (DR_REG_I2C_ANA_MST_BASE + 0x20)
/** REGI2C_CONF2_DCDC_SEL : R/W; bitpos: [12] */
#define REGI2C_CONF2_DCDC_SEL     (BIT(12))
