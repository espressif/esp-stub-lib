/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include "spi_nand.h"

__attribute__((weak)) int spi_nand_init(uint32_t hspi_arg)
{
    (void)hspi_arg;
    return -1; /* NAND not supported on this chip */
}

__attribute__((weak)) int spi_nand_transaction(uint8_t cmd,
                                               const uint8_t *addr,
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
    return -1;
}
