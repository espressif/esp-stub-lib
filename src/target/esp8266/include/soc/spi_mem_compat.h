/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#pragma once

#include <esp-stub-lib/bit_utils.h>

// ESP8266 uses SPI0 as the flash interface
#define SPI_BASE_REG      0x60000200
#define SPI_EXT2_REG      (SPI_BASE_REG + 0xF8)
#define SPI_CMD_REG       (SPI_BASE_REG + 0x00)
#define SPI_RD_STATUS_REG (SPI_BASE_REG + 0x10)
#define SPI_ADDR_REG      (SPI_BASE_REG + 0x04)
#define SPI_FLASH_WREN    BIT(30)
#define SPI_FLASH_RDID    BIT(28)
#define SPI_FLASH_RDSR    BIT(27)
#define SPI_FLASH_SE      BIT(24)
#define SPI_FLASH_BE      BIT(23)
#define SPI_W0            (SPI_BASE_REG + 0x40)
#define SPI_CMD           (SPI_BASE_REG + 0x0)

#define SPI_ST            0x00000007
#define SPI_ST_M          ((SPI_ST_V) << (SPI_ST_S))
#define SPI_ST_V          0x7
#define SPI_ST_S          0

#define SPI_MEM_ADDR_REG(i)       SPI_ADDR_REG
#define SPI_MEM_CMD_REG(i)        SPI_CMD_REG
#define SPI_MEM_RD_STATUS_REG(i)  SPI_RD_STATUS_REG
#define SPI_MEM_FLASH_SE          SPI_FLASH_SE
#define SPI_MEM_FLASH_BE          SPI_FLASH_BE
#define SPI_MEM_FLASH_RDSR        SPI_FLASH_RDSR
