/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <esp-stub-lib/log.h>
#include <target/test_drv.h>

void stub_target_common_test_drv_whoami(void)
{
    STUB_LOGD("I am esp32 test driver\n");
}
