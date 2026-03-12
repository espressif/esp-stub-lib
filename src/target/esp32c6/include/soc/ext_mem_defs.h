/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>

/* Total number of MMU entries on ESP32-C6 */
#define SOC_MMU_ENTRY_NUM     256

/*
 * MMU entry bit fields (stored in SPI_MEM_MMU_ITEM_CONTENT_REG):
 *   bits [8:0]  - physical page number (value)
 *   bit  [9]    - valid bit
 *   bit  [10]   - sensitive (flash encryption) bit
 */
#define SOC_MMU_VALID           BIT(9)
#define SOC_MMU_SENSITIVE       BIT(10)
#define SOC_MMU_INVALID         0
#define SOC_MMU_VALID_VAL_MASK  0x1FFU
