/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void esp_stub_lib_delay_us(uint32_t us);
uint16_t esp_stub_lib_crc16_le(uint16_t crc, const uint8_t *buf, uint32_t len);
void esp_stub_lib_tx_one_char(uint8_t c);
uint8_t esp_stub_lib_rx_one_char(void);
void esp_stub_lib_tx_flush(void);

#ifdef __cplusplus
}
#endif // __cplusplus
