/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <target/uart.h>

// These functions are defined in the ROM
extern void uartAttach(void *rxBuffer);
extern void Uart_Init(uint8_t uart_no, uint32_t clock);
extern uint32_t ets_get_apb_freq(void);
extern void ets_update_cpu_frequency(uint32_t ticks_per_us);

#define UART_CLK_FREQ_ROM (40 * 1000000)

void stub_target_uart_init(uint8_t uart_num, uint32_t baudrate)
{
    (void)baudrate;
    extern bool g_uart_print;
    uartAttach(NULL);
    ets_update_cpu_frequency(ets_get_apb_freq() / 1000000);
    Uart_Init(uart_num, UART_CLK_FREQ_ROM);
    g_uart_print = true;
}

void stub_target_uart_set_baudrate(uint8_t uart_num, uint32_t baudrate)
{
    (void)uart_num;
    (void)baudrate;
    // TODO: implement
}

void stub_target_uart_intr_attach(uint8_t uart_num, int intr_num, void *handler, uint32_t flags)
{
    (void)uart_num;
    (void)intr_num;
    (void)handler;
    (void)flags;
    // TODO: implement
}

uint32_t stub_target_uart_get_intr_flags(uint8_t uart_num)
{
    (void)uart_num;
    // TODO: implement
    return 0;
}

uint32_t stub_target_uart_get_rxfifo_count(uint8_t uart_num)
{
    (void)uart_num;
    // TODO: implement
    return 0;
}

uint8_t stub_target_uart_read_rxfifo_byte(uint8_t uart_num)
{
    (void)uart_num;
    // TODO: implement
    return 0;
}

void stub_target_uart_set_rx_timeout(uint8_t uart_num, uint8_t timeout)
{
    (void)uart_num;
    (void)timeout;
    // TODO: implement
}
