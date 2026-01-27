/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

/* Flash command opcodes */
#define CMD_RDID                 0x9F  /* Read ID */
#define CMD_WRSR                 0x01  /* Write Status Register */
#define CMD_WRDI                 0x04  /* Write Disable */
#define CMD_RDSR                 0x05  /* Read Status Register */
#define CMD_WREN                 0x06  /* Write Enable */
#define CMD_READ                 0x03  /* Read Data */
#define CMD_FSTRD4B              0x0C  /* Fast Read (4-byte address) */
#define CMD_PROGRAM_PAGE         0x02  /* Page Program */
#define CMD_PROGRAM_PAGE_4B      0x12  /* Page Program (4-byte address) */
#define CMD_SECTOR_ERASE         0x20  /* Sector Erase */
#define CMD_SECTOR_ERASE_4B      0x21  /* Sector Erase (4-byte address) */
#define CMD_LARGE_BLOCK_ERASE    0xD8  /* Block Erase (64KB) */
#define CMD_LARGE_BLOCK_ERASE_4B 0xDC  /* Block Erase (64KB, 4-byte address) */

/* OPI Flash chip select masks */
#define ESP_ROM_OPIFLASH_SEL_CS0 (1 << 0)
#define ESP_ROM_OPIFLASH_SEL_CS1 (1 << 1)
