/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <soc/spi_mem_reg.h>

#define PERIPHS_SPI_FLASH_CMD               SPI_MEM_CMD_REG(1)
#define PERIPHS_SPI_FLASH_C0                SPI_MEM_W0_REG(1)

#define PERIPHS_SPI_FLASH_BITS_RDID         SPI_MEM_FLASH_RDID
