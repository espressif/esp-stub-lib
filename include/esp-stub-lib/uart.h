/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <soc/soc_caps.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief UART port number type
 *
 * Available UART ports depend on the chip's SOC_UART_HP_NUM capability.
 * Use the UART_NUM_* enum values (e.g., UART_NUM_0, UART_NUM_1, etc.)
 */

typedef enum {
    UART_NUM_0, /*!< UART port 0 */
    UART_NUM_1, /*!< UART port 1 */
#if SOC_UART_HP_NUM > 2
    UART_NUM_2, /*!< UART port 2 */
#endif
#if SOC_UART_HP_NUM > 3
    UART_NUM_3, /*!< UART port 3 */
#endif
#if SOC_UART_HP_NUM > 4
    UART_NUM_4, /*!< UART port 4 */
#endif
    UART_NUM_MAX, /*!< UART port max */
} uart_port_t;

#define UART_INTR_RXFIFO_FULL (0x1 << 0)
#define UART_INTR_RXFIFO_TOUT (0x1 << 8)

/**
 * @brief Wait until UART is idle
 *
 * @param uart_num UART port number
 */
void stub_lib_uart_wait_idle(uart_port_t uart_num);

/**
 * @brief Initialize UART
 *
 * @param uart_num UART port number
 */
void stub_lib_uart_init(uart_port_t uart_num);

/**
 * @brief Set UART baud rate
 *
 * @param uart_num UART port number
 * @param baudrate Baud rate
 */
void stub_lib_uart_rominit_set_baudrate(uart_port_t uart_num, uint32_t baudrate);

/**
 * @brief Attach interrupt handler to UART and configure interrupts
 *
 * @note This function requires ROM preinit (download mode initialization) to work correctly.
 *       It uses ROM functions (ets_isr_attach, ets_isr_unmask) that are only available
 *       after the ROM has entered download mode and completed its initialization.
 *       Functions that contain "rominit" have this requirement.
 *
 * @param uart_num UART port number
 * @param intr_num CPU interrupt source
 * @param handler Interrupt handler function pointer
 * @param flags Interrupt enable flags (bitwise OR of UART_INTR_* values)
 */
void stub_lib_uart_rominit_intr_attach(uart_port_t uart_num, int intr_num, void *handler, uint32_t flags);

/**
 * @brief Get and clear UART interrupt status
 *
 * This function returns the interrupt status and automatically clears it.
 * Call this at the beginning of your interrupt handler.
 *
 * @param uart_num UART port number
 * @return Bitmask of active interrupts that were cleared
 */
uint32_t stub_lib_uart_clear_intr_flags(uart_port_t uart_num);

/**
 * @brief Get number of bytes available in RX FIFO
 *
 * @param uart_num UART port number
 * @return Number of bytes currently in the RX FIFO
 */
uint32_t stub_lib_uart_get_rxfifo_count(uart_port_t uart_num);

/**
 * @brief Read a single byte from UART RX FIFO
 *
 * This function reads one byte from the RX FIFO without checking
 * if data is available. Call stub_lib_uart_get_rxfifo_count()
 * first to ensure data is available.
 *
 * @param uart_num UART port number
 * @return The byte read from the FIFO
 */
uint8_t stub_lib_uart_read_rxfifo_byte(uart_port_t uart_num);

/**
 * @brief Configure RX timeout threshold
 *
 * @param uart_num UART port number
 * @param timeout Timeout value in bit times (1-126, 0 to disable)
 * timeout mean the multiple of UART packets (~11 bytes) that can be received before timeout
 */
void stub_lib_uart_set_rx_timeout(uart_port_t uart_num, uint8_t timeout);

/**
 * @brief Transmit a single byte over UART.
 *
 * @note This function operates on the console UART (set via stub_lib_uart_set_as_console).
 *       It does not take a uart_num parameter.
 *
 * @param c Byte to transmit.
 * @return 0 if successful, non-zero if error occurred.
 */
uint8_t stub_lib_uart_tx_one_char(uint8_t c);

/**
 * @brief Receive a single byte (blocking) from UART.
 *
 * @note This function operates on the console UART (set via stub_lib_uart_set_as_console).
 *       It does not take a uart_num parameter.
 *
 * @return Received byte.
 */
uint8_t stub_lib_uart_rx_one_char(void);

/**
 * @brief Flush any buffered transmit data.
 *
 * @param uart_no UART port number
 */
void stub_lib_uart_tx_flush(uart_port_t uart_no);

#ifdef __cplusplus
}
#endif // __cplusplus
