/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// These functions are defined in the ROM
extern void uartAttach(void *rxBuffer);
extern void Uart_Init(uint8_t uart_no);
extern uint32_t ets_clk_get_xtal_freq(void);
extern void ets_update_cpu_frequency(uint32_t ticks_per_us);
extern void ets_install_putc1(void (*p)(char c));
extern void ets_install_putc2(void (*p)(char c));

void stub_target_uart_init(uint8_t uart_num, uint32_t baudrate)
{
    (void)baudrate;
    uartAttach(NULL);
    ets_update_cpu_frequency(ets_clk_get_xtal_freq() / 1000000);
    Uart_Init(uart_num);

    ets_install_putc1(NULL);
    ets_install_putc2(NULL);
}
