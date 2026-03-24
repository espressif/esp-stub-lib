/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

/*
 * Stub implementations of NAND target functions.
 *
 * These strong-symbol stubs override the weak fallbacks in
 * esp-stub-lib/src/target/common/src/nand.c. They return sentinel values
 * that allow the plugin dispatch path to be exercised end-to-end without
 * any real NAND hardware or SPI peripheral access.
 */

#include <stdint.h>
#include <string.h>

#include <target/nand.h>

int stub_target_nand_attach(uint32_t hspi_arg)
{
    (void)hspi_arg;
    return 0;
}

uint32_t stub_target_nand_get_page_size(void)
{
    return 2048;
}

int stub_target_nand_read_id(uint8_t *manufacturer_id, uint16_t *device_id)
{
    if (manufacturer_id) {
        *manufacturer_id = 0xAA;
    }
    if (device_id) {
        *device_id = 0x0000;
    }
    return 0;
}

int stub_target_nand_read_register(uint8_t reg, uint8_t *val)
{
    (void)reg;
    if (val) {
        *val = 0x00;
    }
    return 0;
}

int stub_target_nand_read_bbm(uint32_t page_number, uint8_t *spare_data)
{
    (void)page_number;
    if (spare_data) {
        spare_data[0] = 0xFF;
        spare_data[1] = 0xFF;
    }
    return 0;
}

int stub_target_nand_write_bbm(uint32_t page_number, uint8_t is_bad)
{
    (void)page_number;
    (void)is_bad;
    return 0;
}

int stub_target_nand_erase_block(uint32_t page_number)
{
    (void)page_number;
    return 0;
}

int stub_target_nand_read_page(uint32_t page_number, uint8_t *buf, uint32_t buf_size)
{
    (void)page_number;
    if (buf && buf_size > 0) {
        memset(buf, 0xFF, buf_size);
    }
    return 0;
}

int stub_target_nand_write_page(uint32_t page_number, const uint8_t *buf)
{
    (void)page_number;
    (void)buf;
    return NAND_ERR_PROGRAM_FAILED;
}
