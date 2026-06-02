/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <err.h>
#include <target/key_mgr.h>

/*
 * Weak defaults for the Key Manager HAL. Chips that ship a Key Manager
 * (ESP32-C5, ESP32-P4 >= v3.0) provide strong overrides under
 * src/target/<chip>/src/key_mgr.c; chips without KM keep these no-op /
 * safe-default versions so the library always links.
 */

void __attribute__((weak)) stub_target_huk_get_status(uint8_t *gen_status, uint8_t *risk_level)
{
    if (gen_status != NULL) {
        *gen_status = 0U;
    }
    if (risk_level != NULL) {
        *risk_level = STUB_KM_HUK_RISK_ALERT_LEVEL;
    }
}

int __attribute__((weak)) stub_target_huk_configure(stub_huk_mode_t mode, uint8_t *huk_info_buf)
{
    (void)mode;
    (void)huk_info_buf;
    return STUB_LIB_FAIL;
}

void __attribute__((weak)) stub_target_km_bringup(void)
{
}

stub_km_state_t __attribute__((weak)) stub_target_km_get_state(void)
{
    return STUB_KM_STATE_IDLE;
}

void __attribute__((weak)) stub_target_km_wait_for_state(stub_km_state_t state)
{
    (void)state;
}

void __attribute__((weak)) stub_target_km_set_keygen_mode(stub_km_keygen_mode_t mode)
{
    (void)mode;
}

void __attribute__((weak)) stub_target_km_set_key_purpose(stub_km_key_purpose_t purpose)
{
    (void)purpose;
}

void __attribute__((weak)) stub_target_km_use_sw_init_key(void)
{
}

void __attribute__((weak)) stub_target_km_set_xts_aes_key_len(stub_km_key_type_t key_type, bool use_256)
{
    (void)key_type;
    (void)use_256;
}

void __attribute__((weak)) stub_target_km_start(void)
{
}

void __attribute__((weak)) stub_target_km_continue(void)
{
}

void __attribute__((weak)) stub_target_km_write_sw_init_key(const uint8_t *buf, size_t len)
{
    (void)buf;
    (void)len;
}

void __attribute__((weak)) stub_target_km_write_assist_info(const uint8_t *buf, size_t len)
{
    (void)buf;
    (void)len;
}

void __attribute__((weak)) stub_target_km_write_public_info(const uint8_t *buf, size_t len)
{
    (void)buf;
    (void)len;
}

void __attribute__((weak)) stub_target_km_read_assist_info(uint8_t *buf, size_t len)
{
    (void)buf;
    (void)len;
}

void __attribute__((weak)) stub_target_km_read_public_info(uint8_t *buf, size_t len)
{
    (void)buf;
    (void)len;
}

bool __attribute__((weak)) stub_target_km_is_huk_valid(void)
{
    return false;
}

bool __attribute__((weak)) stub_target_km_is_key_deployment_valid(stub_km_key_type_t key_type,
                                                                  stub_km_key_len_t key_len)
{
    (void)key_type;
    (void)key_len;
    return false;
}

void __attribute__((weak)) stub_target_km_set_key_usage(stub_km_key_type_t key_type, bool use_own_key)
{
    (void)key_type;
    (void)use_own_key;
}

bool __attribute__((weak)) stub_target_km_is_efuse_init_key_burned(void)
{
    return false;
}
