/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
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
 * @brief Configure UART RX timeout threshold and enable (SoC-specific layout).
 *
 * Default (weak): UART_TOUT_CONF_SYNC_REG + UART_REG_UPDATE_REG where present.
 * Override on targets that use MEM_CONF / CONF1 only layouts.
 *
 * @param uart_num UART port number
 * @param timeout Threshold (masked per SoC); 0 disables RX timeout.
 */
void stub_target_uart_set_rx_timeout(uint8_t uart_num, uint8_t timeout);

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
