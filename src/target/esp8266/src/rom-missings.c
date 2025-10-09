/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <stdint.h>
#include <soc_utils.h>

void uart_tx_flush(void)
{
    const uint32_t UART0_STATUS_REG = 0x60000F1C;
    const uint32_t UART_STATUS_TX_EMPTY = 0xFF << 16;

    while (REG_READ(UART0_STATUS_REG) & UART_STATUS_TX_EMPTY) {
        // wait for TX fifo to be empty, ROM code works the same way
    }
}
