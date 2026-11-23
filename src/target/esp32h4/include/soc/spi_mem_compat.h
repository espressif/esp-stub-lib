/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#pragma once

#include "spi1_mem_reg.h"
#include "spi_mem_c_reg.h"

/* Program or erase operation flag of SPI_MEM_CMD_REG, which the ROM sets but spi1_mem_reg.h does not name */
#define SPI_MEM_FLASH_PE (BIT(17))
