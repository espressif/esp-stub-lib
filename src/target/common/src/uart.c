/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stddef.h>
#include <stdint.h>

#include <esp-stub-lib/rom_wrappers.h>

#include <target/clock.h>
#include <target/uart.h>

extern void esp_rom_uart_set_as_console(uint8_t uart_no);
extern void esp_rom_uart_tx_wait_idle(uint8_t uart_num);
extern void esp_rom_uart_div_modify(uint8_t uart_no, uint32_t divisor);
extern void esp_rom_uart_flush_tx(uint8_t uart_no);
extern uint32_t esp_rom_get_xtal_freq(void);

void __attribute__((weak)) stub_target_uart_wait_idle(uint8_t uart_num)
{
    esp_rom_uart_tx_wait_idle(uart_num);
}

void __attribute__((weak)) stub_target_uart_init(uint8_t uart_num)
{
    stub_target_rom_uart_attach(NULL);
    stub_target_rom_uart_init(uart_num, esp_rom_get_xtal_freq());
    esp_rom_uart_set_as_console(uart_num);
}

void __attribute__((weak)) stub_target_uart_rominit_set_baudrate(uint8_t uart_num, uint32_t baudrate)
{
    stub_lib_delay_us(5 * 1000);

    uint32_t clk_div = (esp_rom_get_xtal_freq() << 4) / baudrate;
    stub_target_uart_wait_idle(uart_num);
    esp_rom_uart_div_modify(uart_num, clk_div);

    stub_lib_delay_us(5 * 1000);
}

void __attribute__((weak)) stub_target_uart_tx_flush(uint8_t uart_no)
{
    esp_rom_uart_flush_tx(uart_no);
}
