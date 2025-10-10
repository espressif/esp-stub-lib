/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <esp-stub-lib/log.h>
#include <target/test_drv.h>

static int s_test_value = 0;

int __attribute__((weak)) stub_target_common_test_drv_init(void)
{
    STUB_LOGD("stub_target_common_test_drv_init\n");
    s_test_value = 0;
    return 0;
}

int __attribute__((weak)) stub_target_common_test_drv_set(int value)
{
    STUB_LOGD("stub_target_common_test_drv_set set value to %d\n", value);
    s_test_value = value;
    return 0;
}

int __attribute__((weak)) stub_target_common_test_drv_get(void)
{
    STUB_LOGD("stub_target_common_test_drv_get get value %d\n", s_test_value);
    return s_test_value;
}

void __attribute__((weak)) stub_target_common_test_drv_whoami(void)
{

}
