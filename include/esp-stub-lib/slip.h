/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* SLIP Protocol Constants */
#define SLIP_END            0xC0    /* Frame delimiter */
#define SLIP_ESC            0xDB    /* Escape character */
#define SLIP_ESC_END        0xDC    /* Escaped frame delimiter */
#define SLIP_ESC_ESC        0xDD    /* Escaped escape character */

/* SLIP Return Values */
#define SLIP_FINISHED_FRAME    -2   /* Complete frame received */
#define SLIP_NO_BYTE          -1   /* No byte available */

/* SLIP State Machine */
typedef enum {
    SLIP_NO_FRAME,          /* Not in a frame */
    SLIP_FRAME,             /* Processing frame data */
    SLIP_FRAME_ESCAPING     /* Processing escape sequence */
} slip_state_t;

/**
 * @brief Send SLIP frame delimiter
 */
void slip_send_frame_delimiter(void);

/**
 * @brief Send single byte with SLIP escaping
 *
 * @param byte Byte to send
 */
void slip_send_frame_data(uint8_t byte);

/**
 * @brief Send buffer with SLIP escaping
 *
 * @param data Data buffer to send
 * @param size Size of data in bytes
 */
void slip_send_frame_data_buf(const void *data, size_t size);

/**
 * @brief Send complete SLIP frame
 *
 * @param data Data to send
 * @param size Size of data in bytes
 */
void slip_send_frame(const void *data, size_t size);

/**
 * @brief Process incoming byte through SLIP decoder
 *
 * @param byte Incoming byte
 * @param state SLIP decoder state
 * @return Decoded byte (>=0), SLIP_NO_BYTE, or SLIP_FINISHED_FRAME
 */
int16_t slip_recv_byte(uint8_t byte, slip_state_t *state);

/**
 * @brief Receive complete SLIP frame (synchronous)
 *
 * @param buffer Buffer to store received frame
 * @param max_len Maximum buffer size
 * @return Number of bytes received
 */
size_t slip_recv_frame(void *buffer, size_t max_len);

#ifdef __cplusplus
}
#endif
