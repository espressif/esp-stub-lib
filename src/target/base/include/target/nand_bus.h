/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>

/**
 * Enable GPSPI2 and route the NAND pins.
 *
 * @param hspi_arg Packed pin map, or 0 for the chip's default pins.
 *        bits  0..5  CLK, 6..11 Q, 12..17 D, 18..23 CS, 24..29 HD.
 * @return 0 on success, NAND_ERR_PIN_INVALID when a pin number is out of range.
 */
int nand_spi_init(uint32_t hspi_arg);
