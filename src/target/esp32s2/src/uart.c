/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <stddef.h>

// These functions are defined in the ROM
extern void uartAttach(void *rxBuffer);
extern uint32_t ets_get_apb_freq(void);
extern void ets_update_cpu_frequency(uint32_t ticks_per_us);
extern void Uart_Init(uint8_t uart_no, uint32_t clock);
extern void uart_tx_switch(uint8_t uart_no);

void stub_target_uart_init(uint8_t uart_num, uint32_t baudrate)
{
    (void) baudrate;
    uint32_t clock = ets_get_apb_freq();
    uartAttach(NULL);
    ets_update_cpu_frequency(clock / 1000000);
    Uart_Init(uart_num, clock);
    uart_tx_switch(uart_num);
}
