/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include "reg_base.h"

#define EFUSE_RD_REPEAT_DATA1_REG (DR_REG_EFUSE_BASE + 0x34)
#define EFUSE_RD_REPEAT_DATA2_REG (DR_REG_EFUSE_BASE + 0x38)

/*
 * Per-block KEY_PURPOSE field positions (5-bit purpose values, one block per
 * eFuse KEY[0..5]). Used by the Key Manager HAL to scan for KM_INIT_KEY.
 */
#define EFUSE_C5_KEY_PURPOSE_M       0x1FU
#define EFUSE_C5_KEY0_PURPOSE_IN_DATA1_S 22
#define EFUSE_C5_KEY1_PURPOSE_IN_DATA1_S 27
#define EFUSE_C5_KEY2_PURPOSE_IN_DATA2_S 0
#define EFUSE_C5_KEY3_PURPOSE_IN_DATA2_S 5
#define EFUSE_C5_KEY4_PURPOSE_IN_DATA2_S 10
#define EFUSE_C5_KEY5_PURPOSE_IN_DATA2_S 15
