/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

/**
 * @file
 * @brief Error codes used in esp-stub-lib
 *
 * Please use int for return codes
 *
 * The error code range of esp-stub-lib starts at STUB_LIB_ERROR_BASE, intentionally chosen to distinguish library codes
 */

#define STUB_LIB_ERROR_BASE                     0x10000000

#define STUB_LIB_OK                             0                       /**< Success (no error) */
#define STUB_LIB_FAIL                           STUB_LIB_ERROR_BASE     /**< A generic error code. Prefer specific codes instead. */

/** @name Specific error codes
 *  @{
 */
#define STUB_LIB_ERR_UNKNOWN_FLASH_ID           (STUB_LIB_ERROR_BASE + 0x1)
#define STUB_LIB_ERR_FLASH_READ_UNALIGNED       (STUB_LIB_ERROR_BASE + 0x2)
#define STUB_LIB_ERR_FLASH_READ_ROM_ERR         (STUB_LIB_ERROR_BASE + 0x3)
#define STUB_LIB_ERR_NOT_SUPPORTED              (STUB_LIB_ERROR_BASE + 0x4)
#define STUB_LIB_ERR_INVALID_ARG                (STUB_LIB_ERROR_BASE + 0x5)
/** @} */
