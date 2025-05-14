/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include "soc/reg_base.h"

#define REG_SPI_MEM_BASE(i) (DR_REG_SPI0_BASE + (i) * 0x1000) // SPIMEM0 and SPIMEM1
