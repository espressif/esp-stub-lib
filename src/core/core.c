/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <stdlib.h>
#include <esp-stub-lib/core.h>

static struct stub_target s_target;

void __attribute__((weak)) stub_target_test_drv_init(struct stub_target *target)
{
    target->test_drv_ops = NULL;
}

struct stub_target *stub_lib_new_target(const char *target_name)
{
    (void)target_name;

    s_target.target_name = target_name;

    stub_target_test_drv_init(&s_target);

    return &s_target;
}
