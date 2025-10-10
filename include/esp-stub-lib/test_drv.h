/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <esp-stub-lib/core.h>

struct stub_test_drv_ops {
    int (*init)(void);
    int (*set)(int value);
    int (*get)(void);
    void (*whoami)(void);
};

int stub_lib_test_drv_init(struct stub_target *target);
int stub_lib_test_drv_set(struct stub_target *target, int value);
int stub_lib_test_drv_get(struct stub_target *target);
void stub_lib_test_drv_whoami(struct stub_target *target);
