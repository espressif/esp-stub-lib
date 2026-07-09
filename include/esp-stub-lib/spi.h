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

/** Maximum frame length for a single SPI slave-HD DMA transfer.
 *  The ROM DMA loaders (spi_slave_rom_txdma_load / _rxdma_load) reject any
 *  length above LLDESC_SPI_MAX_BUFFER_SIZE (4096 - 4), so this is the true
 *  single-descriptor limit, not the 12-bit descriptor field max (0xFFF). */
#define SPI_DMA_DESC_MAX_LEN (4096U - 4U)

/**
 * @brief Check whether the SPI (download boot) transport is active.
 *
 * @return true if the ROM left GP-SPI2 in slave mode with a valid download
 *         handshake, i.e. this stub was loaded over SPI.
 */
bool stub_lib_spi_is_active(void);

/**
 * @brief Initialize the SPI (download boot) transport.
 * Resets the SPI DMA, initialises credits, and registers the internal SPI2 ISR
 * using ROM interrupt infrastructure. The CPU interrupt number is chosen
 * per-target inside the driver. Call stub_lib_spi_rearm(buf, max_size) to arm
 * receive DMA.
 */
void stub_lib_spi_init(void);

/**
 * @brief Claim a completed received frame from the SPI driver.
 *
 * Returns true once per received frame and writes its byte count to @p out_len.
 * The transport should mark the shared frame buffer complete before calling
 * stub_lib_spi_rearm(buf, max_size).
 */
bool stub_lib_spi_take_rx_frame(size_t *out_len);

/**
 * @brief Arm the receive DMA when it is not already armed.
 *
 * If the receive DMA is already armed, this is a no-op. Call after freeing or
 * claiming a frame buffer to provide the next DMA destination.
 *
 * @param buf      Writable 4-byte-aligned receive buffer.
 * @param max_size Capacity of @p buf in bytes.
 * @return STUB_LIB_OK on success, STUB_LIB_ERR_* on failure.
 */
int stub_lib_spi_rearm(uint8_t *buf, size_t max_size);

/**
 * @brief Send one raw SPI frame.
 *
 * Transfers via the SPI slave HD TX DMA path and waits until the host reads
 * the frame, so stack-backed buffers remain valid for the duration.
 *
 * @param data Pointer to frame bytes (must be 4-byte aligned).
 * @param len  Number of frame bytes, up to the DMA descriptor limit (4092).
 * @return STUB_LIB_OK on success, STUB_LIB_ERR_* on failure.
 */
int stub_lib_spi_tx_frame(const void *data, size_t len);

#ifdef __cplusplus
}
#endif
