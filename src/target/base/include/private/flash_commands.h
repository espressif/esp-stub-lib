/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>

/**
 * @brief Flash command opcodes description
 * - CMD_RDID:                 Read ID
 * - CMD_WRSR:                 Write Status Register
 * - CMD_WRDI:                 Write Disable
 * - CMD_RDSR:                 Read Status Register
 * - CMD_WREN:                 Write Enable
 * - CMD_READ:                 Read Data
 * - CMD_FSTRD4B:              Fast Read (4-byte address)
 * - CMD_PROGRAM_PAGE:         Page Program
 * - CMD_PROGRAM_PAGE_4B:      Page Program (4-byte address)
 * - CMD_SECTOR_ERASE:         Sector Erase
 * - CMD_SECTOR_ERASE_4B:      Sector Erase (4-byte address)
 * - CMD_LARGE_BLOCK_ERASE:    Block Erase (64KB)
 * - CMD_LARGE_BLOCK_ERASE_4B: Block Erase (64KB, 4-byte address)
 */

#define CMD_RDID                 0x9F
#define CMD_WRSR                 0x01
#define CMD_WRDI                 0x04
#define CMD_RDSR                 0x05
#define CMD_WREN                 0x06
#define CMD_READ                 0x03
#define CMD_FSTRD4B              0x0C
#define CMD_PROGRAM_PAGE         0x02
#define CMD_PROGRAM_PAGE_4B      0x12
#define CMD_SECTOR_ERASE         0x20
#define CMD_SECTOR_ERASE_4B      0x21
#define CMD_LARGE_BLOCK_ERASE    0xD8
#define CMD_LARGE_BLOCK_ERASE_4B 0xDC

/* OPI Flash chip select masks */
#define ESP_ROM_OPIFLASH_SEL_CS0 BIT(0)
#define ESP_ROM_OPIFLASH_SEL_CS1 BIT(1)
