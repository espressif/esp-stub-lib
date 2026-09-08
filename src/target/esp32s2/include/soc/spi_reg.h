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
#define SPI_SLAVE_REG        (DR_REG_SPI2_BASE + 0x030) /* slave-mode control         */
#define SPI_SLV_RD_BYTE_REG  (DR_REG_SPI2_BASE + 0x040) /* received byte count (RX)   */
#define SPI_DMA_CONF_REG     (DR_REG_SPI2_BASE + 0x04C) /* SPI-integrated DMA config  */
#define SPI_DMA_OUT_LINK_REG (DR_REG_SPI2_BASE + 0x050) /* TX (RDDMA) outlink control */
#define SPI_DMA_IN_LINK_REG  (DR_REG_SPI2_BASE + 0x054) /* RX (WRDMA) inlink control  */
#define SPI_DMA_INT_RAW_REG  (DR_REG_SPI2_BASE + 0x05C) /* DMA raw interrupt status   */
#define SPI_DMA_INT_CLR_REG  (DR_REG_SPI2_BASE + 0x064) /* DMA interrupt clear        */
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

/* SPI_DMA_CONF_REG fields */
#define SPI_IN_RST                BIT(2)       /* reset the in (RX) DMA FSM and data FIFO  */
#define SPI_OUT_RST               BIT(3)       /* reset the out (TX) DMA FSM and data FIFO */
#define SPI_AHBM_FIFO_RST         BIT(4)       /* reset the DMA AHB-master FIFO pointer    */
#define SPI_AHBM_RST              BIT(5)       /* reset the DMA AHB master                 */

/* SPI_DMA_OUT_LINK_REG fields */
#define SPI_OUTLINK_START         BIT(29)      /* start consuming the outlink descriptor   */
#define SPI_OUTLINK_ADDR          0x000FFFFFU  /* [19:0] address of first outlink desc     */
#define SPI_DMA_TX_ENA            BIT(31)      /* enable the out (TX) DMA path             */

/* SPI_DMA_IN_LINK_REG fields */
#define SPI_INLINK_START          BIT(29)      /* start consuming the inlink descriptor    */
#define SPI_INLINK_ADDR           0x000FFFFFU  /* [19:0] address of first inlink desc      */
#define SPI_DMA_RX_ENA            BIT(31)      /* enable the in (RX) DMA path              */


#ifdef __cplusplus
}
#endif
