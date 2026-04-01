/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * Weak NAND implementation for chips that do not support SPI NAND.
 * Target-specific chips that support NAND provide strong symbols that override these.
 */

#include <esp-stub-lib/err.h>

#include <target/nand.h>

__attribute__((weak)) int stub_target_nand_attach(uint32_t hspi_arg)
{
    (void)hspi_arg;
    return STUB_LIB_ERR_NOT_SUPPORTED;
}

__attribute__((weak)) int stub_target_nand_read_bbm(uint32_t page_number, uint8_t *spare_data)
{
    (void)page_number;
    (void)spare_data;
    return STUB_LIB_ERR_NOT_SUPPORTED;
}

__attribute__((weak)) int stub_target_nand_write_bbm(uint32_t page_number, uint8_t is_bad)
{
    (void)page_number;
    (void)is_bad;
    return STUB_LIB_ERR_NOT_SUPPORTED;
}

__attribute__((weak)) int stub_target_nand_read_id(uint8_t *manufacturer_id, uint16_t *device_id)
{
    (void)manufacturer_id;
    (void)device_id;
    return STUB_LIB_ERR_NOT_SUPPORTED;
}

__attribute__((weak)) int stub_target_nand_read_page(uint32_t page_number, uint8_t *buf, uint32_t buf_size)
{
    (void)page_number;
    (void)buf;
    (void)buf_size;
    return STUB_LIB_ERR_NOT_SUPPORTED;
}

__attribute__((weak)) int stub_target_nand_write_page(uint32_t page_number, const uint8_t *buf, uint32_t buf_size)
{
    (void)page_number;
    (void)buf;
    (void)buf_size;
    return STUB_LIB_ERR_NOT_SUPPORTED;
}

__attribute__((weak)) int stub_target_nand_erase_block(uint32_t page_number)
{
    (void)page_number;
    return STUB_LIB_ERR_NOT_SUPPORTED;
}

__attribute__((weak)) uint32_t stub_target_nand_get_page_size(void)
{
    return 0;
}

__attribute__((weak)) int stub_target_nand_read_register(uint8_t reg, uint8_t *val)
{
    (void)reg;
    (void)val;
    return STUB_LIB_ERR_NOT_SUPPORTED;
}
