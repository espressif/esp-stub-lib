/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <common/target_flash.h>

void stub_target_flash_init(void)
{
    return;
}

uint32_t stub_target_flash_get_flash_id(void)
{
    return 0;
}
