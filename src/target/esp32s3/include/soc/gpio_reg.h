/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#pragma once

#include <soc/reg_base.h>

#define GPIO_ENABLE_W1TS_REG  (DR_REG_GPIO_BASE + 0x024U)
#define GPIO_ENABLE_W1TC_REG  (DR_REG_GPIO_BASE + 0x02CU)
#define GPIO_ENABLE1_W1TS_REG (DR_REG_GPIO_BASE + 0x034U)
#define GPIO_ENABLE1_W1TC_REG (DR_REG_GPIO_BASE + 0x038U)
