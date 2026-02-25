/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>

#include "reg_base.h"

/* ICACHE_CTRL: cache enable */
#define EXTMEM_ICACHE_CTRL_REG            (DR_REG_EXTMEM_BASE + 0x0)
#define EXTMEM_ICACHE_ENABLE              BIT(0)

/* ICACHE_CTRL1: I/D bus shut bits */
#define EXTMEM_ICACHE_CTRL1_REG           (DR_REG_EXTMEM_BASE + 0x4)
#define EXTMEM_ICACHE_SHUT_IBUS           BIT(0)
#define EXTMEM_ICACHE_SHUT_DBUS           BIT(1)

#define EXTMEM_ICACHE_AUTOLOAD_ENA        0 /* autoload is always 0 on ESP32-C2 */

/* CACHE_CONF_MISC: page size field at bits [4:3] */
#define EXTMEM_CACHE_CONF_MISC_REG        (DR_REG_EXTMEM_BASE + 0xc8)
#define EXTMEM_CACHE_MMU_PAGE_SIZE        0x00000003U
#define EXTMEM_CACHE_MMU_PAGE_SIZE_M      (EXTMEM_CACHE_MMU_PAGE_SIZE_V << EXTMEM_CACHE_MMU_PAGE_SIZE_S)
#define EXTMEM_CACHE_MMU_PAGE_SIZE_V      0x00000003U
#define EXTMEM_CACHE_MMU_PAGE_SIZE_S      3
