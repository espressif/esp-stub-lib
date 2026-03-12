/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>

/* Flash MMU entries per core — 384 entries for flash (0..383) */
#define SOC_MMU_ENTRY_NUM  384

/* BIT(8) is the INVALID flag; a valid entry has this bit clear */
#define SOC_MMU_INVALID    BIT(8)
