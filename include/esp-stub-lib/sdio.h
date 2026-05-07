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

/** Maximum frame length supported by SDIO DMA descriptor size/length fields. */
#define SDIO_DMA_DESC_MAX_LEN 0x3FFFU

/**
 * @brief Check if SDIO is the active download transport.
 *
 * Calls ROM sip_download_begin(). Safe to call before stub_lib_sdio_init().
 *
 * @return true if SDIO download mode is active
 */
bool stub_lib_sdio_is_active(void);

/**
 * @brief Initialise SDIO transport and register the receive ISR.
 *
 * Resets the SLC DMA, initialises credits, and registers the internal SLC0 ISR
 * using ROM interrupt infrastructure. The CPU interrupt number is chosen
 * per-target inside the driver. Call stub_lib_sdio_rearm(buf, max_size) to arm
 * receive DMA.
 */
void stub_lib_sdio_init(void);

/**
 * @brief Claim a completed received frame from the SDIO driver.
 *
 * Returns true once per received frame and writes its byte count to @p out_len.
 * The transport should mark the shared frame buffer complete before calling
 * stub_lib_sdio_rearm(buf, max_size).
 */
bool stub_lib_sdio_take_rx_frame(size_t *out_len);

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
int stub_lib_sdio_rearm(uint8_t *buf, size_t max_size);

/**
 * @brief Send one raw SDIO frame.
 *
 * Transfers via SLC slave-to-host DMA and waits until the host consumes the
 * packet, so stack-backed buffers remain valid for the duration.
 *
 * @param data Pointer to frame bytes (must be 4-byte aligned).
 * @param len  Number of frame bytes, up to the DMA descriptor limit (0x3FFF).
 * @return STUB_LIB_OK on success, STUB_LIB_ERR_* on failure.
 */
int stub_lib_sdio_tx_frame(const void *data, size_t len);

#ifdef __cplusplus
}
#endif
