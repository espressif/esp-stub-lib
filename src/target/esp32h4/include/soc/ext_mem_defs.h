/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>

#define SOC_MMU_ENTRY_NUM               512
#define SOC_MMU_ACCESS_FLASH            0
#define SOC_MMU_ACCESS_SPIRAM           BIT(9)
#define SOC_MMU_VALID                   BIT(10)
#define SOC_MMU_SENSITIVE               BIT(11)
#define SOC_MMU_INVALID_MASK            BIT(10)
#define SOC_MMU_INVALID                 0
#define SOC_MMU_VALID_VAL_MASK          (SOC_MMU_ACCESS_SPIRAM - 1)
