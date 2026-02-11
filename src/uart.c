/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stddef.h>
#include <stdint.h>

#include <soc_utils.h>
#include <uart.h>

#include <target/uart.h>

#include <soc/uart_reg.h>

// External ROM functions
extern void esp_rom_isr_attach(int int_num, void *handler, void *arg);
extern void esp_rom_isr_unmask(int int_num);
extern uint8_t esp_rom_uart_tx_one_char(uint8_t ch);
extern uint8_t esp_rom_uart_rx_one_char(void);

void stub_lib_uart_wait_idle(uart_port_t uart_num)
{
    stub_target_uart_wait_idle(uart_num);
}

void stub_lib_uart_init(uart_port_t uart_num)
{
    stub_target_uart_init(uart_num);
}

void stub_lib_uart_rominit_set_baudrate(uart_port_t uart_num, uint32_t baudrate)
{
    stub_target_uart_rominit_set_baudrate(uart_num, baudrate);
}

void stub_lib_uart_tx_flush(uart_port_t uart_no)
{
    stub_target_uart_tx_flush(uart_no);
}

void stub_lib_uart_rominit_intr_attach(uart_port_t uart_num, int intr_num, void *handler, uint32_t flags)
{
    // Clear pending interrupt flags
    WRITE_PERI_REG(UART_INT_CLR_REG(uart_num), 0xFFFFFFFFU);

    esp_rom_isr_attach(intr_num, handler, NULL);

    if (flags != 0U) {
        SET_PERI_REG_MASK(UART_INT_ENA_REG(uart_num), flags);
    }

    esp_rom_isr_unmask(1 << intr_num);
}

uint32_t stub_lib_uart_clear_intr_flags(uart_port_t uart_num)
{
    uint32_t status = READ_PERI_REG(UART_INT_ST_REG(uart_num));
    // Clear the interrupts
    WRITE_PERI_REG(UART_INT_CLR_REG(uart_num), status);
    return status;
}

uint32_t stub_lib_uart_get_rxfifo_count(uart_port_t uart_num)
{
    uint32_t status = READ_PERI_REG(UART_STATUS_REG(uart_num));
    return (status >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT_V;
}

uint8_t stub_lib_uart_read_rxfifo_byte(uart_port_t uart_num)
{
    return (uint8_t)(READ_PERI_REG(UART_FIFO_REG(uart_num)) & UART_RXFIFO_RD_BYTE_V);
}

void stub_lib_uart_set_rx_timeout(uart_port_t uart_num, uint8_t timeout)
{
    uint32_t conf1 = READ_PERI_REG(UART_CONF1_REG(uart_num));
    conf1 &= ~(UART_RX_TOUT_THRHD_M | UART_RX_TOUT_EN_M);

    if (timeout > 0U) {
        conf1 |= (((uint32_t)timeout & UART_RX_TOUT_THRHD_V) << UART_RX_TOUT_THRHD_S) | UART_RX_TOUT_EN;
    }

    WRITE_PERI_REG(UART_CONF1_REG(uart_num), conf1);
}

uint8_t stub_lib_uart_tx_one_char(uint8_t c)
{
    return esp_rom_uart_tx_one_char(c);
}

uint8_t stub_lib_uart_rx_one_char(void)
{
    return esp_rom_uart_rx_one_char();
}
