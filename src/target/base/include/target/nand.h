/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>

#include <esp-stub-lib/nand.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enable the SPI master and route the NAND pins
 *
 * @param pins Explicit bus pins, or NULL for all target defaults. HD and WP may be STUB_LIB_NAND_PIN_NC.
 * @return STUB_LIB_OK on success, STUB_LIB_ERR_INVALID_ARG when a pin is out of range,
 *         STUB_LIB_ERR_NOT_SUPPORTED when the target has no NAND support.
 */
int stub_target_nand_bus_init(const stub_lib_nand_pins_t *pins);

/**
 * @brief Run one half-duplex SPI transaction
 *
 * Sends an 8-bit command, an optional address and optional data, then reads optional data.
 * A transaction moves at most 64 data bytes.
 *
 * @param cmd       Command byte
 * @param addr      Address, sent MSB first
 * @param addr_bits Address length in bits, 0 for none
 * @param tx_data   Data to send, may be NULL when tx_bits is 0
 * @param tx_bits   Data to send in bits
 * @param rx_data   Buffer for received data, may be NULL when rx_bits is 0
 * @param rx_bits   Data to receive in bits
 * @return STUB_LIB_OK on success, STUB_LIB_ERR_NAND_SPI on a bus timeout
 */
int stub_target_nand_bus_transaction(uint8_t cmd,
                                     uint32_t addr,
                                     uint8_t addr_bits,
                                     const uint8_t *tx_data,
                                     uint16_t tx_bits,
                                     uint8_t *rx_data,
                                     uint16_t rx_bits);

#ifdef __cplusplus
}
#endif
