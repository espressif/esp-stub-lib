/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

int stub_lib_test_drv_init(void);
int stub_lib_test_drv_set(int value);
int stub_lib_test_drv_get(void);
void stub_lib_test_drv_whoami(void);

#ifdef __cplusplus
}
#endif // __cplusplus
