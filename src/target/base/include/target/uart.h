/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>

/**
 * @brief Wait until UART is idle
 *
 * @param uart_num UART port number
 */
void stub_target_uart_wait_idle(uint8_t uart_num);

/**
 * @brief Initialize UART
 *
 * @param uart_num UART port number
 */
void stub_target_uart_init(uint8_t uart_num);

/**
 * @brief Set UART baud rate
 *
 * @param uart_num UART port number
 * @param baudrate Baud rate
 */
void stub_target_uart_rominit_set_baudrate(uint8_t uart_num, uint32_t baudrate);

/**
 * @brief Flush any buffered transmit data.
 */
void stub_target_uart_tx_flush(uint8_t uart_no);

/**
 * @brief Wrapper for esp_rom_uart_attach
 *
 * Each target must implement this to adapt to its ROM function signature.
 * @param rxBuffer RX buffer pointer (may be ignored by some targets)
 */
void stub_target_rom_uart_attach(void *rxBuffer);

/**
 * @brief Wrapper for esp_rom_uart_init
 *
 * Each target must implement this to adapt to its ROM function signature.
 * @param uart_no UART number
 * @param clock Clock frequency (may be calculated internally by some targets)
 */
void stub_target_rom_uart_init(uint8_t uart_no, uint32_t clock);
