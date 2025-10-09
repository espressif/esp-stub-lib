/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <bit_utils.h>

#define REG_UART_BASE(i)                (0x60000000 + (i)*0xf00)
//version value:32'h062000

#define UART_FIFO_REG(i)                    (REG_UART_BASE(i) + 0x0)
#define UART_RXFIFO_RD_BYTE_V               0x000000FF
#define UART_RXFIFO_RD_BYTE_S               0

#define UART_INT_RAW_REG(i)                 (REG_UART_BASE(i) + 0x4)
#define UART_RXFIFO_TOUT_INT_RAW            (BIT(8))
#define UART_BRK_DET_INT_RAW                (BIT(7))
#define UART_CTS_CHG_INT_RAW                (BIT(6))
#define UART_DSR_CHG_INT_RAW                (BIT(5))
#define UART_RXFIFO_OVF_INT_RAW             (BIT(4))
#define UART_FRM_ERR_INT_RAW                (BIT(3))
#define UART_PARITY_ERR_INT_RAW             (BIT(2))
#define UART_TXFIFO_EMPTY_INT_RAW           (BIT(1))
#define UART_RXFIFO_FULL_INT_RAW            (BIT(0))

#define UART_INT_ST_REG(i)                  (REG_UART_BASE(i) + 0x8)
#define UART_RXFIFO_TOUT_INT_ST             (BIT(8))
#define UART_BRK_DET_INT_ST                 (BIT(7))
#define UART_CTS_CHG_INT_ST                 (BIT(6))
#define UART_DSR_CHG_INT_ST                 (BIT(5))
#define UART_RXFIFO_OVF_INT_ST              (BIT(4))
#define UART_FRM_ERR_INT_ST                 (BIT(3))
#define UART_PARITY_ERR_INT_ST              (BIT(2))
#define UART_TXFIFO_EMPTY_INT_ST            (BIT(1))
#define UART_RXFIFO_FULL_INT_ST             (BIT(0))

#define UART_INT_ENA_REG(i)                 (REG_UART_BASE(i) + 0xC)
#define UART_RXFIFO_TOUT_INT_ENA            (BIT(8))
#define UART_BRK_DET_INT_ENA                (BIT(7))
#define UART_CTS_CHG_INT_ENA                (BIT(6))
#define UART_DSR_CHG_INT_ENA                (BIT(5))
#define UART_RXFIFO_OVF_INT_ENA             (BIT(4))
#define UART_FRM_ERR_INT_ENA                (BIT(3))
#define UART_PARITY_ERR_INT_ENA             (BIT(2))
#define UART_TXFIFO_EMPTY_INT_ENA           (BIT(1))
#define UART_RXFIFO_FULL_INT_ENA            (BIT(0))

#define UART_INT_CLR_REG(i)                 (REG_UART_BASE(i) + 0x10)
#define UART_RXFIFO_TOUT_INT_CLR            (BIT(8))
#define UART_BRK_DET_INT_CLR                (BIT(7))
#define UART_CTS_CHG_INT_CLR                (BIT(6))
#define UART_DSR_CHG_INT_CLR                (BIT(5))
#define UART_RXFIFO_OVF_INT_CLR             (BIT(4))
#define UART_FRM_ERR_INT_CLR                (BIT(3))
#define UART_PARITY_ERR_INT_CLR             (BIT(2))
#define UART_TXFIFO_EMPTY_INT_CLR           (BIT(1))
#define UART_RXFIFO_FULL_INT_CLR            (BIT(0))

#define UART_RXFIFO_FULL_INT_ENA            (BIT(0))
#define UART_RXFIFO_FULL_INT_ENA_M          (BIT(0))
#define UART_RXFIFO_FULL_INT_ST_M           (BIT(0))
#define UART_RXFIFO_FULL_INT_CLR_M          (BIT(0))

#define UART_TXFIFO_EMPTY_INT_ENA           (BIT(1))
#define UART_TXFIFO_EMPTY_INT_ENA_M         (BIT(1))
#define UART_TXFIFO_EMPTY_INT_ST_M          (BIT(1))
#define UART_TXFIFO_EMPTY_INT_CLR_M         (BIT(1))

#define UART_PARITY_ERR_INT_ENA             (BIT(2))
#define UART_PARITY_ERR_INT_ENA_M           (BIT(2))
#define UART_PARITY_ERR_INT_ST_M            (BIT(2))
#define UART_PARITY_ERR_INT_CLR_M           (BIT(2))

#define UART_FRM_ERR_INT_ENA                (BIT(3))
#define UART_FRM_ERR_INT_ENA_M              (BIT(3))
#define UART_FRM_ERR_INT_ST_M               (BIT(3))
#define UART_FRM_ERR_INT_CLR_M              (BIT(3))

#define UART_RXFIFO_OVF_INT_ENA             (BIT(4))
#define UART_RXFIFO_OVF_INT_ENA_M           (BIT(4))
#define UART_RXFIFO_OVF_INT_ST_M            (BIT(4))
#define UART_RXFIFO_OVF_INT_CLR_M           (BIT(4))

#define UART_RXFIFO_TOUT_INT_ENA            (BIT(8))
#define UART_RXFIFO_TOUT_INT_ENA_M          (BIT(8))
#define UART_RXFIFO_TOUT_INT_ST_M           (BIT(8))
#define UART_RXFIFO_TOUT_INT_CLR_M          (BIT(8))

#define UART_CLKDIV_REG(i)                  (REG_UART_BASE(i) + 0x14)
#define UART_CLKDIV_M                       (UART_CLKDIV_V << UART_CLKDIV_S)
#define UART_CLKDIV_CNT_V                   0x000FFFFF
#define UART_CLKDIV_S                       0

#define UART_AUTOBAUD_REG(i)                (REG_UART_BASE(i) + 0x18)
#define UART_GLITCH_FILT_V                  0x000000FF
#define UART_GLITCH_FILT_S                  8
#define UART_AUTOBAUD_EN                    (BIT(0))

#define UART_STATUS_REG(i)                  (REG_UART_BASE(i) + 0x1C)
#define UART_TXD                            (BIT(31))
#define UART_RTSN                           (BIT(30))
#define UART_DTRN                           (BIT(29))
#define UART_TXFIFO_CNT_V                   0x000000FF
#define UART_TXFIFO_CNT_S                   16
#define UART_RXD                            (BIT(15))
#define UART_CTSN                           (BIT(14))
#define UART_DSRN                           (BIT(13))
#define UART_RXFIFO_CNT_V                   0x000000FF
#define UART_RXFIFO_CNT_S                   0

#define UART_CONF0_REG(i)                   (REG_UART_BASE(i) + 0x20)
#define UART_DTR_INV                        (BIT(24))
#define UART_RTS_INV                        (BIT(23))
#define UART_TXD_INV                        (BIT(22))
#define UART_DSR_INV                        (BIT(21))
#define UART_CTS_INV                        (BIT(20))
#define UART_RXD_INV                        (BIT(19))
#define UART_TXFIFO_RST                     (BIT(18))
#define UART_RXFIFO_RST                     (BIT(17))
#define UART_IRDA_EN                        (BIT(16))
#define UART_TX_FLOW_EN                     (BIT(15))
#define UART_LOOPBACK                       (BIT(14))
#define UART_IRDA_RX_INV                    (BIT(13))
#define UART_IRDA_TX_INV                    (BIT(12))
#define UART_IRDA_WCTL                      (BIT(11))
#define UART_IRDA_TX_EN                     (BIT(10))
#define UART_IRDA_DPLX                      (BIT(9))
#define UART_TXD_BRK                        (BIT(8))
#define UART_SW_DTR                         (BIT(7))
#define UART_SW_RTS                         (BIT(6))
#define UART_STOP_BIT_NUM_V                 0x00000003
#define UART_STOP_BIT_NUM_S                 4
#define UART_BIT_NUM_V                      0x00000003
#define UART_BIT_NUM_S                      2
#define UART_PARITY_EN                      (BIT(1))
#define UART_PARITY_EN_M                0x00000001
#define UART_PARITY_EN_S                 1
#define UART_PARITY                         (BIT(0))
#define UART_PARITY_M                       0x00000001
#define UART_PARITY_S                        0

#define UART_CONF1_REG(i)                   (REG_UART_BASE(i) + 0x24)
#define UART_RX_TOUT_EN                     (BIT(31))
#define UART_RX_TOUT_EN_M                   (BIT(31))
#define UART_RX_TOUT_THRHD_M                (UART_RX_TOUT_THRHD_V << UART_RX_TOUT_THRHD_S)
#define UART_RX_TOUT_THRHD_V                0x0000007F
#define UART_RX_TOUT_THRHD_S                24
#define UART_RX_FLOW_EN                     (BIT(23))
#define UART_RX_FLOW_EN_M                   (BIT(23))
#define UART_RX_FLOW_THRHD_V                0x0000007F
#define UART_RX_FLOW_THRHD_S                16
#define UART_TXFIFO_EMPTY_THRHD_M            (UART_TXFIFO_EMPTY_THRHD_V << UART_TXFIFO_EMPTY_THRHD_S)
#define UART_TXFIFO_EMPTY_THRHD_V           0x0000007F
#define UART_TXFIFO_EMPTY_THRHD_S           8
#define UART_RXFIFO_FULL_THRHD_M            (UART_RXFIFO_FULL_THRHD_V << UART_RXFIFO_FULL_THRHD_S)
#define UART_RXFIFO_FULL_THRHD_V            0x0000007F
#define UART_RXFIFO_FULL_THRHD_S            0

#define UART_LOWPULSE_REG(i)                (REG_UART_BASE(i) + 0x28)
#define UART_LOWPULSE_MIN_CNT_M             (UART_LOWPULSE_MIN_CNT_V << UART_LOWPULSE_MIN_CNT_S)
#define UART_LOWPULSE_MIN_CNT_V             0x000FFFFF
#define UART_LOWPULSE_MIN_CNT_S             0

#define UART_HIGHPULSE_REG(i)               (REG_UART_BASE(i) + 0x2C)
#define UART_HIGHPULSE_MIN_CNT_M            (UART_HIGHPULSE_MIN_CNT_V << UART_HIGHPULSE_MIN_CNT_S)
#define UART_HIGHPULSE_MIN_CNT_V            0x000FFFFF
#define UART_HIGHPULSE_MIN_CNT_S            0

#define UART_PULSE_NUM_REG(i)               (REG_UART_BASE(i) + 0x30)
#define UART_PULSE_NUM_CNT_M                (UART_PULSE_NUM_CNT_V << UART_PULSE_NUM_CNT_S)
#define UART_PULSE_NUM_CNT_V                0x0003FF
#define UART_PULSE_NUM_CNT_S                0

#define UART_DATE_REG(i)                    (REG_UART_BASE(i) + 0x78)
#define UART_ID_REG(i)                      (REG_UART_BASE(i) + 0x7C)

#define UART_SWAP_REG                       0x3FF00028

#ifdef __cplusplus
}
#endif  /* end of __cplusplus */
