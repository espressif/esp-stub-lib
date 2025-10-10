/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <esp-stub-lib/core.h>
#include <esp-stub-lib/log.h>
#include <esp-stub-lib/test_drv.h>

static void esp32_test_drv_whoami(void)
{
    STUB_LOGD("I am esp32 test driver\n");
}

static const struct stub_test_drv_ops s_esp32_test_drv_ops = {
    .whoami = esp32_test_drv_whoami,
};

void stub_target_test_drv_init(struct stub_target *target)
{
    target->test_drv_ops = (void *)&s_esp32_test_drv_ops;
}
