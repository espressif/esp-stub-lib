/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <esp-stub-lib/core.h>
#include <esp-stub-lib/test_drv.h>
#include <target/test_drv.h>

int stub_lib_test_drv_init(struct stub_target *target)
{
    struct stub_test_drv_ops *ops = target->test_drv_ops;

    if (ops && ops->init) {
        return ops->init();
    }

    return stub_target_common_test_drv_init();
}

int stub_lib_test_drv_set(struct stub_target *target, int value)
{
    struct stub_test_drv_ops *ops = target->test_drv_ops;

    if (ops && ops->set) {
        return ops->set(value);
    }

    // Fallback to common implementation
    return stub_target_common_test_drv_set(value);
}

int stub_lib_test_drv_get(struct stub_target *target)
{
    struct stub_test_drv_ops *ops = target->test_drv_ops;

    if (ops && ops->get) {
        return ops->get();
    }

    return stub_target_common_test_drv_get();
}

void stub_lib_test_drv_whoami(struct stub_target *target)
{
    struct stub_test_drv_ops *ops = target->test_drv_ops;

    if (ops && ops->whoami) {
        ops->whoami();
    }

    stub_target_common_test_drv_whoami();
}
