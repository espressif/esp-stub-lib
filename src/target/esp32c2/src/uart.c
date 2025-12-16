/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <target/uart.h>
#include <target/clock.h>
#include <esp-stub-lib/rom_wrappers.h>
#include <soc_utils.h>
#include <soc/uart_reg.h>

// These functions are defined in the ROM
extern void esp_rom_uart_attach(void *rxBuffer);
extern void esp_rom_uart_init(uint8_t uart_no, uint32_t clock);
extern void esp_rom_uart_set_as_console(uint8_t uart_no);
extern uint32_t esp_rom_get_xtal_freq(void);
extern void esp_rom_uart_div_modify(uint8_t uart_no, uint32_t divisor);
extern void esp_rom_isr_attach(int int_num, void *handler, void *arg);
extern void esp_rom_isr_unmask(int int_num);

void stub_target_rom_uart_attach(void *rxBuffer)
{
    esp_rom_uart_attach(rxBuffer);
}

void stub_target_rom_uart_init(uint8_t uart_no, uint32_t clock)
{
    esp_rom_uart_init(uart_no, clock);
}

void stub_target_uart_init(uint8_t uart_num)
{
    // Bug for some esp32c2 revisions, does not work with rev v1.0.
    extern bool g_uart_print;
    stub_target_rom_uart_attach(NULL);
    stub_target_rom_uart_init(uart_num, stub_target_get_apb_freq());
    esp_rom_uart_set_as_console(uart_num);
    g_uart_print = true;
}

void stub_target_uart_rominit_set_baudrate(uint8_t uart_num, uint32_t baudrate)
{
    stub_lib_delay_us(5 * 1000);

    uint32_t clk_div = (stub_target_get_apb_freq() << 4) / baudrate;
    stub_target_uart_wait_idle(uart_num);
    esp_rom_uart_div_modify(uart_num, clk_div);

    stub_lib_delay_us(5 * 1000);
}
