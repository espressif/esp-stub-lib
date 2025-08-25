/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

typedef int stub_lib_err_t;

#define STUB_LIB_OK                             0       // stub_lib_err_t value indicating success (no error)
#define STUB_LIB_FAIL                           -1      // Generic stub_lib_err_t code indicating failure

#define STUB_LIB_ERR_UNKNOWN_FLASH_ID           0x1
