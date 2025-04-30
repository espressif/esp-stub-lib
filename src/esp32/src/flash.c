/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <common/log.h>
#include "flash.h"

void stub_target_flash_init(void *state)
{
    (void)state;

    STUB_LOG_TRACE();
}

void stub_target_flash_deinit(const void *state)
{
    (void)state;
    STUB_LOG_TRACE();
}
