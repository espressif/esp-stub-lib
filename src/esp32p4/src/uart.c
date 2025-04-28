/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <stddef.h>

// These functions are defined in the ROM
extern void uartAttach(void *rxBuffer);
extern void Uart_Init(uint8_t uart_no);
extern uint32_t ets_clk_get_cpu_freq(void);
extern void ets_update_cpu_frequency(uint32_t ticks_per_us);
extern void uart_tx_switch(uint8_t uart_no);

void stub_target_uart_init(uint8_t uart_num, uint32_t baudrate)
{
    (void)baudrate;
    ets_update_cpu_frequency(ets_clk_get_cpu_freq() / 1000000);
    uartAttach(NULL);
    Uart_Init(uart_num);
    uart_tx_switch(uart_num);
}
