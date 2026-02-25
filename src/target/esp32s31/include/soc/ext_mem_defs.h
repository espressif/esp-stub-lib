/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>

#define SOC_DROM_LOW            0x40000000

#define SOC_MMU_FLASH_VALID         BIT(12)
#define SOC_MMU_FLASH_INVALID       0
#define SOC_MMU_FLASH_SENSITIVE     BIT(13)
#define SOC_MMU_ACCESS_FLASH        0
#define SOC_MMU_ACCESS_PSRAM        BIT(10)
#define SOC_MMU_ENTRY_NUM           1024
#define SOC_MMU_FLASH_VALID_VAL_MASK 0x7ff
