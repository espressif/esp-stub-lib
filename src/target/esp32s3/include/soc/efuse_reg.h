/*
 * SPDX-FileCopyrightText: 2021-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <soc/reg_base.h>

#define EFUSE_RD_MAC_SPI_SYS_3_REG   (DR_REG_EFUSE_BASE + 0x50)
#define EFUSE_RD_MAC_SPI_SYS_4_REG   (DR_REG_EFUSE_BASE + 0x54)
#define EFUSE_RD_MAC_SPI_SYS_5_REG   (DR_REG_EFUSE_BASE + 0x58)
#define EFUSE_RD_SYS_PART1_DATA4_REG (DR_REG_EFUSE_BASE + 0x6C)

#define EFUSE_BLK_VERSION_MINOR_V    0x7U
#define EFUSE_BLK_VERSION_MINOR_S    24

#define EFUSE_BLK_VERSION_MAJOR_V    0x3U
#define EFUSE_BLK_VERSION_MAJOR_S    0

#define EFUSE_K_RTC_LDO_V            0x7FU
#define EFUSE_K_RTC_LDO_S            13

#define EFUSE_K_DIG_LDO_V            0x7FU
#define EFUSE_K_DIG_LDO_S            20

#define EFUSE_V_RTC_DBIAS20_V        0x1FU
#define EFUSE_V_RTC_DBIAS20_S        27

#define EFUSE_V_RTC_DBIAS20_1_V      0x7U
#define EFUSE_V_RTC_DBIAS20_1_S      0

#define EFUSE_V_DIG_DBIAS20_V        0xFFU
#define EFUSE_V_DIG_DBIAS20_S        3

#define EFUSE_DIG_DBIAS_HVT_V        0x1FU
#define EFUSE_DIG_DBIAS_HVT_S        11
