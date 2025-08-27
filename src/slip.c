/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <slip.h>
#include <log.h>

/* External ROM UART functions */
extern void uart_tx_one_char(uint8_t ch);
extern uint8_t uart_rx_one_char(void);
extern void uart_tx_flush(void);

void slip_send_frame_delimiter(void)
{
    uart_tx_one_char(SLIP_END);
}

void slip_send_frame_data(uint8_t byte)
{
    switch (byte) {
    case SLIP_END:
        uart_tx_one_char(SLIP_ESC);
        uart_tx_one_char(SLIP_ESC_END);
        break;
    case SLIP_ESC:
        uart_tx_one_char(SLIP_ESC);
        uart_tx_one_char(SLIP_ESC_ESC);
        break;
    default:
        uart_tx_one_char(byte);
        break;
    }
}

void slip_send_frame_data_buf(const void *data, size_t size)
{
    if (!data) {
        return;
    }

    const uint8_t *buf = (const uint8_t *)data;
    for (size_t i = 0; i < size; i++) {
        slip_send_frame_data(buf[i]);
    }
}

void slip_send_frame(const void *data, size_t size)
{
    if (!data) {
        return;
    }

    slip_send_frame_delimiter();
    slip_send_frame_data_buf(data, size);
    slip_send_frame_delimiter();
}

int16_t slip_recv_byte(uint8_t byte, slip_state_t *state)
{
    if (!state) {
        return SLIP_NO_BYTE;
    }

    if (byte == SLIP_END) {
        if (*state == SLIP_NO_FRAME) {
            *state = SLIP_FRAME;
            return SLIP_NO_BYTE;
        } else {
            *state = SLIP_NO_FRAME;
            return SLIP_FINISHED_FRAME;
        }
    }

    switch (*state) {
    case SLIP_NO_FRAME:
        return SLIP_NO_BYTE;

    case SLIP_FRAME:
        if (byte == SLIP_ESC) {
            *state = SLIP_FRAME_ESCAPING;
            return SLIP_NO_BYTE;
        }
        return (int16_t)byte;

    case SLIP_FRAME_ESCAPING:
        *state = SLIP_FRAME;
        switch (byte) {
        case SLIP_ESC_END:
            return SLIP_END;
        case SLIP_ESC_ESC:
            return SLIP_ESC;
        default:
            /* Framing error - ignore invalid escape sequence */
            return SLIP_NO_BYTE;
        }
    }

    return SLIP_NO_BYTE;
}

size_t slip_recv_frame(void *buffer, size_t max_len)
{
    if (!buffer) {
        return 0;
    }

    size_t len = 0;
    slip_state_t state = SLIP_NO_FRAME;
    uint8_t *buf = (uint8_t *)buffer;

    while (len < max_len) {
        uint8_t byte = uart_rx_one_char();
        int16_t result = slip_recv_byte(byte, &state);

        if (result == SLIP_FINISHED_FRAME) {
            break;
        } else if (result >= 0) {
            buf[len++] = (uint8_t)result;
        }
    }

    return len;
}
