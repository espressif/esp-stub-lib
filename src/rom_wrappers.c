/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <stdint.h>

extern void ets_delay_us(uint32_t us);
extern uint16_t crc16_le(uint16_t crc, const uint8_t *buf, uint32_t len);

void esp_stub_lib_delay_us(uint32_t us)
{
    ets_delay_us(us);
}

uint16_t esp_stub_lib_crc16_le(uint16_t crc, const uint8_t *buf, uint32_t len)
{
    return crc16_le(crc, buf, len);
}
