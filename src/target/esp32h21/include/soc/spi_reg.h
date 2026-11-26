/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#pragma once

#include <esp-stub-lib/bit_utils.h>
#include <soc/reg_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* GP-SPI2 subset needed by the SPI-slave DMA path, named as in the ESP-IDF
 * ESP32-H21 soc/spi_reg.h. */

#define DR_REG_SPI_BASE(i) (((i) == 2) ? (DR_REG_GPSPI2_BASE) : (0)) /* only one GPSPI */

/* DMA control. The *_AFIFO_RST bits are write-trigger and read back as 0. */
#define SPI_DMA_CONF_REG(i) (DR_REG_SPI_BASE(i) + 0x30)
#define SPI_DMA_RX_ENA (BIT(27))
#define SPI_DMA_TX_ENA (BIT(28))
#define SPI_RX_AFIFO_RST (BIT(29))
#define SPI_DMA_AFIFO_RST (BIT(31))

/* DMA interrupts */
#define SPI_DMA_INT_ENA_REG(i) (DR_REG_SPI_BASE(i) + 0x34)

#define SPI_DMA_INT_CLR_REG(i) (DR_REG_SPI_BASE(i) + 0x38)
#define SPI_SLV_RD_DMA_DONE_INT_CLR (BIT(8))
#define SPI_SLV_WR_DMA_DONE_INT_CLR (BIT(9))

#define SPI_DMA_INT_RAW_REG(i) (DR_REG_SPI_BASE(i) + 0x3c)
#define SPI_SLV_RD_DMA_DONE_INT_RAW (BIT(8))
#define SPI_SLV_WR_DMA_DONE_INT_RAW (BIT(9))

/* Shared data buffer words */
#define SPI_W0_REG(i) (DR_REG_SPI_BASE(i) + 0x98)
#define SPI_W1_REG(i) (DR_REG_SPI_BASE(i) + 0x9c)
#define SPI_W2_REG(i) (DR_REG_SPI_BASE(i) + 0xa0)
#define SPI_W3_REG(i) (DR_REG_SPI_BASE(i) + 0xa4)

/* Slave mode */
#define SPI_SLAVE_REG(i) (DR_REG_SPI_BASE(i) + 0xe0)
#define SPI_SLAVE_MODE (BIT(26))

#define SPI_SLAVE1_REG(i) (DR_REG_SPI_BASE(i) + 0xe4)
#define SPI_SLV_DATA_BITLEN 0x0003FFFFU

#ifdef __cplusplus
}
#endif
