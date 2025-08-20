/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

// Limit error code range for stub-lib
#define STUB_LIB_ERR_END                            0x100000

#define STUB_LIB_ERROR_CODE_LIST \
    X(STUB_LIB_OK,                                  0,                              "Operation esp-stub-lib success") \
    X(STUB_LIB_FAIL,                                (STUB_LIB_ERR_END - 1),         "Generic esp-stub-lib failure") \
    \
    X(STUB_LIB_ERR_FLASH_INIT_UNKNOWN_FLASH_ID,     0x1000,                         "Unknown flash size, unknown flash id") \

// Only numerical values, used in esp-stub-lib sources

#define X(code_, value_, info_) code_ = value_,
typedef enum stub_lib_err_codes {
    STUB_LIB_ERROR_CODE_LIST
} stub_lib_err_t;
#undef X
