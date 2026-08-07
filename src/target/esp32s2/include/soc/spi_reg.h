/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * Minimal ESP32-S2 GP-SPI2 register definitions for the SPI download-boot
 * transport. Only the registers/fields used by the SPI slave-HD data-plane
 * driver are included; see the ESP32-S2 TRM "SPI Controller" chapter (and the
 * full IDF soc/spi_reg.h) for the complete register map. Offsets are relative
 * to DR_REG_SPI2_BASE and match the canonical IDF definitions.
 */
#pragma once

#include <esp-stub-lib/bit_utils.h>
#include <soc/reg_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Register offsets (GP-SPI2)
 * ------------------------------------------------------------------------- */
#define SPI_SLAVE_REG       (DR_REG_SPI2_BASE + 0x030) /* slave-mode control        */
#define SPI_SLV_RD_BYTE_REG (DR_REG_SPI2_BASE + 0x040) /* received byte count (RX)  */
#define SPI_DMA_INT_RAW_REG (DR_REG_SPI2_BASE + 0x05C) /* DMA raw interrupt status  */
#define SPI_DMA_INT_CLR_REG (DR_REG_SPI2_BASE + 0x064) /* DMA interrupt clear        */
#define SPI_W0_REG          (DR_REG_SPI2_BASE + 0x098) /* shared reg: VER            */
#define SPI_W1_REG          (DR_REG_SPI2_BASE + 0x09C) /* shared reg: RXSTA          */
#define SPI_W2_REG          (DR_REG_SPI2_BASE + 0x0A0) /* shared reg: TXSTA          */
#define SPI_W3_REG          (DR_REG_SPI2_BASE + 0x0A4) /* shared reg: CMD            */

/* -------------------------------------------------------------------------
 * Register fields
 * ------------------------------------------------------------------------- */
#define SPI_SLAVE_MODE            BIT(30)      /* SPI_SLAVE_REG: peripheral is a slave     */
#define SPI_SLV_DATA_BYTELEN      0x000FFFFFU  /* SPI_SLV_RD_BYTE_REG[19:0], bytes         */
#define SPI_IN_SUC_EOF_INT_RAW    BIT(5)       /* SPI_DMA_INT_RAW: WRDMA frame received    */
#define SPI_OUT_TOTAL_EOF_INT_RAW BIT(8)       /* SPI_DMA_INT_RAW: RDDMA frame copied out  */

#ifdef __cplusplus
}
#endif
