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

/* GDMA channel-0 subset needed by the SPI-slave DMA path.
 * C2 shares one INT_RAW/INT_CLR register between the in and out engines. */

/* Interrupt status (shared by in/out) */
#define GDMA_INT_RAW_CH0_REG (DR_REG_GDMA_BASE + 0x0)
#define GDMA_IN_SUC_EOF_CH0_INT_RAW (BIT(1))
#define GDMA_OUT_DONE_CH0_INT_RAW (BIT(3))

#define GDMA_INT_CLR_CH0_REG (DR_REG_GDMA_BASE + 0xc)
#define GDMA_IN_SUC_EOF_CH0_INT_CLR (BIT(1))
#define GDMA_OUT_DONE_CH0_INT_CLR (BIT(3))
#define GDMA_INFIFO_UDF_CH0_INT_CLR (BIT(10))

/* Rx (IN) channel 0 */
#define GDMA_IN_CONF0_CH0_REG (DR_REG_GDMA_BASE + 0x70)
#define GDMA_IN_RST_CH0 (BIT(0))

#define GDMA_IN_LINK_CH0_REG (DR_REG_GDMA_BASE + 0x80)
#define GDMA_INLINK_ADDR_CH0 0x000FFFFFU
#define GDMA_INLINK_START_CH0 (BIT(22))

/* Tx (OUT) channel 0 */
#define GDMA_OUT_CONF0_CH0_REG (DR_REG_GDMA_BASE + 0xd0)
#define GDMA_OUT_RST_CH0 (BIT(0))

#define GDMA_OUT_LINK_CH0_REG (DR_REG_GDMA_BASE + 0xe0)
#define GDMA_OUTLINK_START_CH0 (BIT(21))

#ifdef __cplusplus
}
#endif
