/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct stub_target {
    const char *target_name;
    void *test_drv_ops;
};

struct stub_target *stub_lib_new_target(const char *target_name);

#ifdef __cplusplus
}
#endif // __cplusplus
