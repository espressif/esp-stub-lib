/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "soc_caps.h"

/**
 * @brief UART port numbers
 *
 * Available UART ports depend on the chip's SOC_UART_HP_NUM capability.
 */
typedef enum {
    UART_NUM_0,                         /*!< UART port 0 */
    UART_NUM_1,                         /*!< UART port 1 */
#if SOC_UART_HP_NUM > 2
    UART_NUM_2,                         /*!< UART port 2 */
#endif
#if SOC_UART_HP_NUM > 3
    UART_NUM_3,                         /*!< UART port 3 */
#endif
#if SOC_UART_HP_NUM > 4
    UART_NUM_4,                         /*!< UART port 4 */
#endif
    UART_NUM_MAX,                       /*!< UART port max */
} uart_port_t;

#define UART_INTR_RXFIFO_FULL      (0x1<<0)
#define UART_INTR_RXFIFO_TOUT      (0x1<<8)

/**
 * @brief Initialize UART
 *
 * @param uart_num UART port number
 * @param baudrate Baud rate
 */
void stub_target_uart_init(uint8_t uart_num, uint32_t baudrate);

/**
 * @brief Set UART baud rate
 *
 * @param uart_num UART port number
 * @param baudrate Baud rate
 */
void stub_target_uart_set_baudrate(uint8_t uart_num, uint32_t baudrate);

/**
 * @brief Attach interrupt handler to UART and configure interrupts
 *
 * @param uart_num UART port number
 * @param intr_num CPU interrupt source
 * @param handler Interrupt handler function pointer
 * @param flags Interrupt enable flags (bitwise OR of UART_INTR_* values)
 */
void stub_target_uart_intr_attach(uint8_t uart_num, int intr_num, void *handler, uint32_t flags);

/**
 * @brief Get and clear UART interrupt status
 *
 * This function returns the interrupt status and automatically clears it.
 * Call this at the beginning of your interrupt handler.
 *
 * @param uart_num UART port number
 * @return Bitmask of active interrupts that were cleared
 */
uint32_t stub_target_uart_get_intr_flags(uint8_t uart_num);

/**
 * @brief Get number of bytes available in RX FIFO
 *
 * @param uart_num UART port number
 * @return Number of bytes currently in the RX FIFO
 */
uint32_t stub_target_uart_get_rxfifo_count(uint8_t uart_num);

/**
 * @brief Read a single byte from UART RX FIFO
 *
 * This function reads one byte from the RX FIFO without checking
 * if data is available. Call stub_target_uart_get_rxfifo_count()
 * first to ensure data is available.
 *
 * @param uart_num UART port number
 * @return The byte read from the FIFO
 */
uint8_t stub_target_uart_read_rxfifo_byte(uint8_t uart_num);

/**
 * @brief Configure RX timeout threshold
 *
 * @param uart_num UART port number
 * @param timeout Timeout value in bit times (1-126, 0 to disable)
 * timeout mean the multiple of UART packets (~11 bytes) that can be received before timeout
 */
void stub_target_uart_set_rx_timeout(uint8_t uart_num, uint8_t timeout);
