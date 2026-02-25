/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>

#define SOC_MMU_ENTRY_NUM       256
#define SOC_MMU_VALID           BIT(9)
#define SOC_MMU_SENSITIVE       BIT(10)
#define SOC_MMU_INVALID         0U
#define SOC_MMU_VALID_VAL_MASK  0x1FFU
