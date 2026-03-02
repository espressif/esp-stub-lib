/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * Weak NAND implementation for chips that do not support SPI NAND.
 * ESP32-S3 and future NAND-capable chips provide strong symbols in their target.
 */

#include "nand.h"

static const uint8_t s_debug_id[3] = { 0 };
static const uint8_t s_debug_extra[3] = { 0 };

__attribute__((weak)) int nand_attach(uint32_t hspi_arg)
{
    (void)hspi_arg;
    return -1;
}

__attribute__((weak)) int nand_read_spare(uint32_t page_number, uint8_t *spare_data)
{
    (void)page_number;
    (void)spare_data;
    return -1;
}

__attribute__((weak)) int nand_write_spare(uint32_t page_number, uint8_t is_bad)
{
    (void)page_number;
    (void)is_bad;
    return -1;
}

__attribute__((weak)) int nand_read_id(uint8_t *manufacturer_id, uint16_t *device_id)
{
    (void)manufacturer_id;
    (void)device_id;
    return -1;
}

__attribute__((weak)) int nand_read_page(uint32_t page_number, uint8_t *buf, uint32_t buf_size)
{
    (void)page_number;
    (void)buf;
    (void)buf_size;
    return -1;
}

__attribute__((weak)) int nand_write_page(uint32_t page_number, const uint8_t *buf)
{
    (void)page_number;
    (void)buf;
    return -1;
}

__attribute__((weak)) int nand_erase_block(uint32_t page_number)
{
    (void)page_number;
    return -1;
}

__attribute__((weak)) uint32_t nand_get_page_size(void)
{
    return 0;
}

__attribute__((weak)) const uint8_t *nand_get_debug_id(void)
{
    return s_debug_id;
}

__attribute__((weak)) const uint8_t *nand_get_debug_extra(void)
{
    return s_debug_extra;
}
