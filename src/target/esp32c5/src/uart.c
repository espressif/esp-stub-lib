/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <target/uart.h>

// ROM functions for targets using common implementation
extern void esp_rom_uart_attach(void *rxBuffer);
extern void esp_rom_uart_init(uint8_t uart_no);

void stub_target_rom_uart_attach(void *rxBuffer)
{
    esp_rom_uart_attach(rxBuffer);
}

void stub_target_rom_uart_init(uint8_t uart_no, uint32_t clock)
{
    (void)clock; // Ignore clock parameter for 1-param ROM function
    esp_rom_uart_init(uart_no);
}
