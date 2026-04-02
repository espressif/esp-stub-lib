/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>

#define SOC_MMU_ENTRY_NUM               1024

#define SOC_MMU_FLASH_VALID             BIT(12)
#define SOC_MMU_FLASH_INVALID           0
#define SOC_MMU_PSRAM_VALID             BIT(11)
#define SOC_MMU_PSRAM_INVALID           0

#define SOC_MMU_FLASH_VALID_VAL_MASK    0x7ff
#define SOC_MMU_PSRAM_VALID_VAL_MASK    0x3ff
