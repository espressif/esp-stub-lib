/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>

#define SOC_MMU_ENTRY_NUM       384
#define SOC_MMU_VALID           0
#define SOC_MMU_INVALID         BIT(14)
#define SOC_MMU_ACCESS_FLASH    BIT(15)

#define PRO_CACHE_IBUS2_MMU_START       0x200
#define PRO_CACHE_IBUS2_MMU_END         0x300

#define SOC_MMU_DROM0_PAGES_START   (PRO_CACHE_IBUS2_MMU_START / sizeof(uint32_t))  /* 128 */
#define SOC_MMU_DROM0_PAGES_END     (PRO_CACHE_IBUS2_MMU_END / sizeof(uint32_t))    /* 192 */
