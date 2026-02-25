/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>

#define SOC_MMU_VALID               0
#define SOC_MMU_INVALID             BIT(8)

#define SOC_MMU_ENTRY_NUM           384

#define SOC_MMU_DROM0_PAGES_START   0
#define SOC_MMU_DROM0_PAGES_END     64
