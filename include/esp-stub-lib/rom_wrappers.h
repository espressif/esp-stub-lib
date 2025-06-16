/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>

void esp_stub_lib_delay_us(uint32_t us);
uint16_t esp_stub_lib_crc16_le(uint16_t crc, const uint8_t *buf, uint32_t len);
