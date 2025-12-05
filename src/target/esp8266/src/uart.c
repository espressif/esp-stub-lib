/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*/

#include <stdint.h>
#include <stddef.h>
#include <target/uart.h>
#include <soc_utils.h>
#include <soc/uart_reg.h>

// These functions are defined in the ROM
extern void esp_rom_uart_attach(void);
extern void esp_rom_uart_init(uint8_t uart_no, uint32_t clock);
extern void esp_rom_uart_rx_intr_handler(void *para);
extern void esp_rom_isr_attach(int int_num, void *handler, void *arg);
extern void esp_rom_isr_unmask(int int_num);
extern void esp_rom_uart_div_modify(uint8_t uart_no, uint32_t divisor);
extern uint32_t ets_get_detected_xtal_freq(void);
extern uint32_t esp_rom_get_cpu_ticks_per_us(void);
extern void esp_rom_set_cpu_ticks_per_us(uint32_t ticks_per_us);
extern uint32_t g_ticks_per_us;

void stub_target_rom_uart_attach(void *rxBuffer)
{
    (void)rxBuffer;  // ESP8266 ROM doesn't take parameter
    esp_rom_uart_attach();
}

void stub_target_rom_uart_init(uint8_t uart_no, uint32_t clock)
{
    esp_rom_uart_init(uart_no, clock);
}

void stub_target_uart_wait_idle(uint8_t uart_num)
{
    // Wait for TX FIFO to be empty and TX to be idle
    uint32_t status;
    do {
        status = READ_PERI_REG(UART_STATUS_REG(uart_num));
        // Check if TX FIFO is empty (TXFIFO_CNT == 0) and TX is idle (!TXD)
    } while (((status >> UART_TXFIFO_CNT_S) & UART_TXFIFO_CNT_V) != 0 ||
             (status & UART_TXD) != 0);
}

void stub_target_uart_init(uint8_t uart_num)
{
    (void)uart_num;
    // TODO: implement
}

void stub_target_uart_rominit_set_baudrate(uint8_t uart_num, uint32_t baudrate)
{
    uint32_t xtal_freq = g_ticks_per_us * 1000000;
    uint32_t clock = xtal_freq << 4;
    uint32_t divisor = clock / baudrate;
    esp_rom_uart_div_modify(uart_num, divisor);
}

void stub_target_uart_tx_flush(uint8_t uart_no)
{
    (void)uart_no;
    const uint32_t UART0_STATUS_REG = 0x60000F1C;
    const uint32_t UART_STATUS_TX_EMPTY = 0xFF << 16;

    while (REG_READ(UART0_STATUS_REG) & UART_STATUS_TX_EMPTY) {
        // wait for TX fifo to be empty, ROM code works the same way
    }
}
