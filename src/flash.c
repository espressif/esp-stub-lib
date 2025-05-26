/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <flash.h>
#include <err.h>
#include <target/flash.h>

stub_lib_err_t stub_lib_flash_init(void **state)
{
    stub_target_flash_init(state);
    return STUB_LIB_OK;
}

void stub_lib_flash_deinit(const void *state)
{
    stub_target_flash_deinit(state);
}

stub_lib_err_t stub_lib_flash_get_info(stub_lib_flash_info_t *info)
{
    (void)info;
    // TODO: implement
    return STUB_LIB_OK;
}
