/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#pragma once

#include "spi_mem_c_reg.h"
#include "spi1_mem_c_reg.h"

#define SPI_MEM_RD_STATUS_REG(FLASH_SPI_NUM) SPI1_MEM_C_RD_STATUS_REG
#define SPI_MEM_CMD_REG(FLASH_SPI_NUM)      SPI1_MEM_C_CMD_REG
#define SPI_MEM_ADDR_REG(FLASH_SPI_NUM)     SPI1_MEM_C_ADDR_REG
#define SPI_MEM_FLASH_SE                    SPI1_MEM_C_FLASH_SE
#define SPI_MEM_FLASH_BE                    SPI1_MEM_C_FLASH_BE
#define SPI_MEM_FLASH_RDSR                  SPI1_MEM_C_FLASH_RDSR

#define SPI_MEM_MST_ST                      SPI1_MEM_C_MST_ST
#define SPI_MEM_MST_ST_S                    SPI1_MEM_C_MST_ST_S
#define SPI_MEM_MST_ST_V                    SPI1_MEM_C_MST_ST_V

#define SPI_MEM_SLV_ST                      SPI1_MEM_C_SLV_ST
#define SPI_MEM_SLV_ST_S                    SPI1_MEM_C_SLV_ST_S
#define SPI_MEM_SLV_ST_V                    SPI1_MEM_C_SLV_ST_V
