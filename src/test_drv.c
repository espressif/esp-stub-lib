/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <esp-stub-lib/test_drv.h>
#include <target/test_drv.h>

int stub_lib_test_drv_init(void)
{
    return stub_target_common_test_drv_init();
}

int stub_lib_test_drv_set(int value)
{
    return stub_target_common_test_drv_set(value);
}

int stub_lib_test_drv_get(void)
{
    return stub_target_common_test_drv_get();
}

void stub_lib_test_drv_whoami(void)
{
    stub_target_common_test_drv_whoami();
}
