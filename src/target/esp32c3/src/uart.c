/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <target/uart.h>
#include <soc_utils.h>
#include <soc/uart_reg.h>

// These functions are defined in the ROM
extern void esp_rom_uart_attach(void *rxBuffer);
extern void esp_rom_uart_init(uint8_t uart_no, uint32_t clock);
extern uint32_t esp_rom_get_apb_freq(void);
extern void esp_rom_set_cpu_ticks_per_us(uint32_t ticks_per_us);
extern void esp_rom_uart_div_modify(uint8_t uart_no, uint32_t divisor);
extern void esp_rom_isr_attach(int int_num, void *handler, void *arg);
extern void esp_rom_isr_unmask(int int_num);
extern uint32_t esp_rom_get_xtal_freq(void);

#define UART_CLK_FREQ_ROM (40 * 1000000)

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
    extern bool g_uart_print;
    stub_target_rom_uart_attach(NULL);
    esp_rom_set_cpu_ticks_per_us(esp_rom_get_xtal_freq() / 1000000);
    stub_target_rom_uart_init(uart_num, UART_CLK_FREQ_ROM);
    g_uart_print = true;
}

void stub_target_uart_rominit_set_baudrate(uint8_t uart_num, uint32_t baudrate)
{
    uint32_t clock = esp_rom_get_xtal_freq() << 4;
    uint32_t divisor = clock / baudrate;
    esp_rom_uart_div_modify(uart_num, divisor);
    // TODO: Consider decimal part
}
