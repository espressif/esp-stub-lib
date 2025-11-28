/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <target/uart.h>

extern void esp_rom_set_cpu_ticks_per_us(uint32_t ticks_per_us);
extern uint32_t esp_rom_get_cpu_freq(void);
extern void esp_rom_uart_set_as_console(uint8_t uart_no);
extern void esp_rom_uart_tx_wait_idle(uint8_t uart_num);
extern void esp_rom_uart_div_modify(uint8_t uart_no, uint32_t divisor);
extern void esp_rom_uart_flush_tx(void);

void __attribute__((weak)) stub_target_uart_wait_idle(uint8_t uart_num)
{
    esp_rom_uart_tx_wait_idle(uart_num);
}

void __attribute__((weak)) stub_target_uart_init(uint8_t uart_num)
{
    esp_rom_set_cpu_ticks_per_us(esp_rom_get_cpu_freq() / 1000000);
    stub_target_rom_uart_attach(NULL);
    uint32_t clock = esp_rom_get_cpu_freq();
    stub_target_rom_uart_init(uart_num, clock);
    esp_rom_uart_set_as_console(uart_num);
}

void __attribute__((weak)) stub_target_uart_rominit_set_baudrate(uint8_t uart_num, uint32_t baudrate)
{
    uint32_t clock = esp_rom_get_cpu_freq() << 4;
    uint32_t divisor = clock / baudrate;
    stub_target_uart_wait_idle(uart_num);
    esp_rom_uart_div_modify(uart_num, divisor);
    // TODO: Consider decimal part
}

void __attribute__((weak)) stub_target_uart_tx_flush(void)
{
    esp_rom_uart_flush_tx();
}
