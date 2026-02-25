/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include "reg_base.h"

/* Flash MMU indirect access (SPI_MEM_C / SPI0) */
#define SPI_MEM_C_MMU_ITEM_CONTENT_REG  (DR_REG_FLASH_SPI0_BASE + 0x37c)
#define SPI_MEM_C_MMU_ITEM_INDEX_REG    (DR_REG_FLASH_SPI0_BASE + 0x380)

/* PSRAM MMU indirect access (SPI_MEM_S / PSRAM_MSPI0) */
#define SPI_MEM_S_MMU_ITEM_CONTENT_REG  (DR_REG_PSRAM_MSPI0_BASE + 0x37c)
#define SPI_MEM_S_MMU_ITEM_INDEX_REG    (DR_REG_PSRAM_MSPI0_BASE + 0x380)
