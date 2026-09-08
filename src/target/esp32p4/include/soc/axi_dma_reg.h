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

/* AXI-DMA (GDMA) channel-0 subset needed by the SPI-slave DMA path. */

/* Rx (IN) channel 0 */
#define AXI_DMA_IN_INT_RAW_CH0_REG (DR_REG_AXI_DMA_BASE + 0x0)
#define AXI_DMA_IN_SUC_EOF_CH0_INT_RAW (BIT(1))

#define AXI_DMA_IN_INT_CLR_CH0_REG (DR_REG_AXI_DMA_BASE + 0xc)
#define AXI_DMA_IN_SUC_EOF_CH0_INT_CLR (BIT(1))
#define AXI_DMA_IN_DSCR_ERR_CH0_INT_CLR (BIT(3))
#define AXI_DMA_INFIFO_L1_UDF_CH0_INT_CLR (BIT(6))
#define AXI_DMA_INFIFO_L2_UDF_CH0_INT_CLR (BIT(8))
#define AXI_DMA_INFIFO_L3_UDF_CH0_INT_CLR (BIT(10))

#define AXI_DMA_IN_CONF0_CH0_REG (DR_REG_AXI_DMA_BASE + 0x10)
#define AXI_DMA_IN_RST_CH0 (BIT(0))
#define AXI_DMA_INDSCR_BURST_EN_CH0 (BIT(9))

#define AXI_DMA_IN_LINK1_CH0_REG (DR_REG_AXI_DMA_BASE + 0x20)
#define AXI_DMA_INLINK_STOP_CH0 (BIT(1))
#define AXI_DMA_INLINK_START_CH0 (BIT(2))

#define AXI_DMA_IN_LINK2_CH0_REG (DR_REG_AXI_DMA_BASE + 0x24)

/* Tx (OUT) channel 0 */
#define AXI_DMA_OUT_INT_RAW_CH0_REG (DR_REG_AXI_DMA_BASE + 0x138)
#define AXI_DMA_OUT_DONE_CH0_INT_RAW (BIT(0))

#define AXI_DMA_OUT_INT_CLR_CH0_REG (DR_REG_AXI_DMA_BASE + 0x144)
#define AXI_DMA_OUT_DONE_CH0_INT_CLR (BIT(0))

#define AXI_DMA_OUT_CONF0_CH0_REG (DR_REG_AXI_DMA_BASE + 0x148)
#define AXI_DMA_OUT_RST_CH0 (BIT(0))
#define AXI_DMA_OUT_EOF_MODE_CH0 (BIT(3))
#define AXI_DMA_OUTDSCR_BURST_EN_CH0 (BIT(10))

#define AXI_DMA_OUT_LINK1_CH0_REG (DR_REG_AXI_DMA_BASE + 0x158)
#define AXI_DMA_OUTLINK_STOP_CH0 (BIT(0))
#define AXI_DMA_OUTLINK_START_CH0 (BIT(1))

#define AXI_DMA_OUT_LINK2_CH0_REG (DR_REG_AXI_DMA_BASE + 0x15c)

#ifdef __cplusplus
}
#endif
