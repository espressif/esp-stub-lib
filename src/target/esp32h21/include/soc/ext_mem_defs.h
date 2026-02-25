/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>

/* 256 MMU entries */
#define SOC_MMU_ENTRY_NUM   256

/* BIT(9) is the VALID flag; an entry is invalid when this bit is clear */
#define SOC_MMU_VALID       BIT(9)
#define SOC_MMU_INVALID     0U
