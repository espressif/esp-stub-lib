/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>

/* 512 MMU entries */
#define SOC_MMU_ENTRY_NUM   512

/* BIT(10) is the VALID flag; an entry is invalid when this bit is clear */
#define SOC_MMU_VALID       BIT(10)
#define SOC_MMU_INVALID     0U
