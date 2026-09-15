/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>

#include "reg_base.h"

/*
 * Analog I2C master, the bus used to reach the on-chip analog blocks (here the
 * DC-DC converter).
 */

/* The analog I2C master is clock gated out of reset. */
#define MODEM_LPCON_CLK_CONF_REG  (DR_REG_MODEM_LPCON_BASE + 0x8)
#define MODEM_LPCON_CLK_I2C_MST_EN BIT(2)

/* Two control registers, one per I2C master; the active one is selected below. */
#define I2C_ANA_MST_I2C0_CTRL_REG (DR_REG_I2C_ANA_MST_BASE + 0x0)
#define I2C_ANA_MST_I2C_CTRL_REG(n) (I2C_ANA_MST_I2C0_CTRL_REG + (n) * 4)

#define REGI2C_RTC_SLAVE_ID_V     0xFF
#define REGI2C_RTC_SLAVE_ID_S     0
#define REGI2C_RTC_ADDR_V         0xFF
#define REGI2C_RTC_ADDR_S         8
#define REGI2C_RTC_DATA_V         0xFF
#define REGI2C_RTC_DATA_S         16
#define REGI2C_RTC_WR_CNTL_V      0x1
#define REGI2C_RTC_WR_CNTL_S      24
#define REGI2C_RTC_BUSY           BIT(25)

/* Slave select. The DC-DC converter sits behind the PMU select bit. */
#define I2C_ANA_MST_ANA_CONF1_REG (DR_REG_I2C_ANA_MST_BASE + 0x1C)
#define I2C_ANA_MST_ANA_CONF1_V   0xFFFFFF
#define I2C_ANA_MST_ANA_CONF1_S   0
#define I2C_ANA_MST_ANA_CONF1_M   ((I2C_ANA_MST_ANA_CONF1_V) << (I2C_ANA_MST_ANA_CONF1_S))
#define REGI2C_CONF1_PMU_SEL      BIT(10)

#define I2C_ANA_MST_ANA_CONF2_REG (DR_REG_I2C_ANA_MST_BASE + 0x20)
#define REGI2C_CONF2_PMU_SEL      BIT(12)
