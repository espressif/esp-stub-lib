/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Check whether the SPI (download boot) transport is active.
 *
 * @return true if the ROM left GP-SPI2 in slave mode with a valid download
 *         handshake, i.e. this stub was loaded over SPI.
 */
bool stub_lib_slave_spi_is_active(void);

/**
 * @brief Initialize the SPI (download boot) transport.
 *
 * Disables the ROM's SPI2 interrupt handler so it cannot race the stub's
 * polled handshake, resets the shared CMD register to IDLE (the ROM leaves it
 * as DONE), and reinitialises internal sequence counters.
 * Call stub_lib_slave_spi_rearm(buf, max_size) afterwards to arm receive DMA.
 */
void stub_lib_slave_spi_init(void);

/**
 * @brief Claim a completed received frame from the SPI driver.
 *
 * Returns true once per received frame and writes its byte count to @p out_len.
 * The transport should mark the shared frame buffer complete before calling
 * stub_lib_slave_spi_rearm(buf, max_size).
 */
bool stub_lib_slave_spi_take_rx_frame(size_t *out_len);

/**
 * @brief Arm the receive DMA when it is not already armed.
 *
 * If the receive DMA is already armed, this is a no-op. Call after freeing or
 * claiming a frame buffer to provide the next DMA destination.
 *
 * @param buf      Writable receive buffer, cache-line-aligned on targets with
 *                 cached DMA (e.g. 64 bytes on ESP32-P4), 4-byte-aligned otherwise.
 * @param max_size Capacity of @p buf in bytes.
 * @return STUB_LIB_OK on success, STUB_LIB_ERR_* on failure.
 */
int stub_lib_slave_spi_rearm(uint8_t *buf, size_t max_size);

/**
 * @brief Send one raw SPI frame.
 *
 * Transfers via the SPI slave HD TX DMA path and waits until the host reads
 * the frame, so stack-backed buffers remain valid for the duration.
 *
 * @param data Pointer to frame bytes, cache-line-aligned on targets with
 *             cached DMA (e.g. 64 bytes on ESP32-P4), 4-byte-aligned otherwise.
 * @param len  Number of frame bytes, up to the DMA descriptor limit.
 * @return STUB_LIB_OK on success, STUB_LIB_ERR_* on failure.
 */
int stub_lib_slave_spi_tx_frame(const void *data, size_t len);

#ifdef __cplusplus
}
#endif
