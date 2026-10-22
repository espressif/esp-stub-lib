/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * Weak NAND bus for chips that do not support SPI NAND.
 * Target-specific chips that support NAND provide strong symbols that override these.
 */

#include <esp-stub-lib/err.h>

#include <target/nand.h>

__attribute__((weak)) int stub_target_nand_bus_init(const stub_lib_nand_pins_t *pins)
{
    (void)pins;
    return STUB_LIB_ERR_NOT_SUPPORTED;
}

__attribute__((weak)) int stub_target_nand_bus_transaction(uint8_t cmd,
                                                           uint32_t addr,
                                                           uint8_t addr_bits,
                                                           const uint8_t *tx_data,
                                                           uint16_t tx_bits,
                                                           uint8_t *rx_data,
                                                           uint16_t rx_bits)
{
    (void)cmd;
    (void)addr;
    (void)addr_bits;
    (void)tx_data;
    (void)tx_bits;
    (void)rx_data;
    (void)rx_bits;
    return STUB_LIB_ERR_NOT_SUPPORTED;
}
