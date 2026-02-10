/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

// Commands
enum esp_stub_cmd {
    ESP_STUB_CMD_TEST1 = 0x00,
    ESP_STUB_CMD_TEST2 = 0x01,
    ESP_STUB_CMD_FLASH_MAX_ID = 0x01,
};

// Stub return codes, all compatible with esp-stub-lib/err.h without overlap

#define ESP_STUB_OK                    0x0 // Equal to STUB_LIB_OK
#define ESP_STUB_FAIL                  0x1 // Different from STUB_LIB_FAIL

#define ESP_STUB_ERR_CMD_NOT_SUPPORTED 0x2
