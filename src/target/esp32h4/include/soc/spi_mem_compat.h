/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#pragma once

#include "spi1_mem_reg.h"
#include "spi_mem_c_reg.h"

/* H4 has no FLASH_PE on SE/BE; keep the bit 0 so common erase_cmd can OR it unconditionally. */
#define SPI_MEM_FLASH_PE          0
