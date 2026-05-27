/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * Public API for the Key Manager peripheral on ESP32-P4 (>= v3.0) and
 * ESP32-C5 (any revision).
 *
 * This header is the contract between the keymanager stub plugin and the
 * chip-specific HAL implementation. The plugin uses these accessors to
 * drive the Key Manager + HUK Generator state machines and to move bytes
 * between host RAM and the KM's internal memory blocks.
 *
 * The API intentionally mirrors the names used in ESP-IDF's
 * `components/esp_security/src/esp_key_mgr.c` so that the plugin can be
 * line-by-line compared against the reference driver. Locks, logging,
 * eFuse helpers, and OS plumbing from the IDF version are dropped — the
 * stub runs single-threaded with no RTOS.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- Constants ---------------------------------------------------- */

#define STUB_KM_K2_INFO_SIZE           64
#define STUB_KM_K1_ENCRYPTED_SIZE      32
#define STUB_KM_ECDH0_INFO_SIZE        64  /* k1*G or k2*G as LE-x || LE-y */
#define STUB_KM_KEY_RECOVERY_INFO_SIZE 64  /* per slot, before CRC */
#define STUB_KM_HUK_INFO_SIZE          660 /* Matches IDF HUK_INFO_LEN in rom/km.h */
#define STUB_KM_SW_INIT_KEY_SIZE       32
#define STUB_KM_HUK_RISK_ALERT_LEVEL   4

/* Magic for the on-flash key_recovery_info partition (matches IDF). */
#define STUB_KM_KEY_HUK_SECTOR_MAGIC   0xDEA5CE5AU

/* ---------- Enums (match IDF wire types) -------------------------------- */

typedef enum {
    STUB_KM_STATE_IDLE = 0,
    STUB_KM_STATE_LOAD = 1,
    STUB_KM_STATE_GAIN = 2,
    STUB_KM_STATE_BUSY = 3,
} stub_km_state_t;

typedef enum {
    STUB_KM_KEYGEN_MODE_RANDOM = 0,
    STUB_KM_KEYGEN_MODE_AES = 1,
    STUB_KM_KEYGEN_MODE_ECDH0 = 2,
    STUB_KM_KEYGEN_MODE_ECDH1 = 3,
    STUB_KM_KEYGEN_MODE_RECOVER = 4,
    STUB_KM_KEYGEN_MODE_EXPORT = 5,
} stub_km_keygen_mode_t;

typedef enum {
    STUB_KM_KEY_PURPOSE_INVALID = 0,
    STUB_KM_KEY_PURPOSE_ECDSA_192 = 1,
    STUB_KM_KEY_PURPOSE_ECDSA_256 = 2,
    STUB_KM_KEY_PURPOSE_FLASH_256_1 = 3,
    STUB_KM_KEY_PURPOSE_FLASH_256_2 = 4,
    STUB_KM_KEY_PURPOSE_FLASH_128 = 5,
    STUB_KM_KEY_PURPOSE_HMAC = 6,
    STUB_KM_KEY_PURPOSE_DS = 7,
    STUB_KM_KEY_PURPOSE_PSRAM_256_1 = 8,
    STUB_KM_KEY_PURPOSE_PSRAM_256_2 = 9,
    STUB_KM_KEY_PURPOSE_PSRAM_128 = 10,
    STUB_KM_KEY_PURPOSE_ECDSA_384_L = 11,
    STUB_KM_KEY_PURPOSE_ECDSA_384_H = 12,
} stub_km_key_purpose_t;

typedef enum {
    STUB_KM_KEY_TYPE_ECDSA = 0,
    STUB_KM_KEY_TYPE_FLASH_XTS_AES = 1,
    STUB_KM_KEY_TYPE_HMAC = 2,
    STUB_KM_KEY_TYPE_DS = 3,
    STUB_KM_KEY_TYPE_PSRAM_XTS_AES = 4,
} stub_km_key_type_t;

typedef enum {
    STUB_KM_KEY_LEN_INVALID = 0,
    STUB_KM_KEY_LEN_ECDSA_192,
    STUB_KM_KEY_LEN_ECDSA_256,
    STUB_KM_KEY_LEN_ECDSA_384,
    STUB_KM_KEY_LEN_XTS_AES_128,
    STUB_KM_KEY_LEN_XTS_AES_256,
} stub_km_key_len_t;

typedef enum {
    STUB_HUK_MODE_RECOVER = 0,
    STUB_HUK_MODE_GENERATE = 1,
} stub_huk_mode_t;

/* ---------- HUK Generator ----------------------------------------------- */

/**
 * @brief Read HUK_STATUS_REG.
 *
 * @param[out] gen_status   0=not generated, 1=valid, 2=invalid, 3=reserved
 * @param[out] risk_level   0..7 (higher = more PUF-SRAM error bits; 7 = invalid)
 */
void stub_target_huk_get_status(uint8_t *gen_status, uint8_t *risk_level);

/**
 * @brief Drive the HUK Generator state machine through one cycle.
 *
 * In GENERATE mode, populates @p huk_info_buf with the freshly generated
 * 384-byte HUK info. In RECOVER mode, @p huk_info_buf is loaded into the
 * HUK and the function returns ESP_OK once the HUK is valid.
 *
 * @return 0 on success; negative if HUK could not be brought to a valid
 * state (for RECOVER) or if HUK generation failed (for GENERATE).
 */
int stub_target_huk_configure(stub_huk_mode_t mode, uint8_t *huk_info_buf);

/* ---------- Key Manager bring-up ---------------------------------------- */

/**
 * @brief Enable the KM bus clock, release reset, and power up KM memory.
 *
 * Must be called once before any other stub_target_km_* call. Idempotent.
 * Without it, KM register writes silently no-op (peripheral held in reset)
 * and KEYMNG_*_MEM reads return zero (memory power gate off).
 */
void stub_target_km_bringup(void);

/* ---------- Key Manager state machine ----------------------------------- */

stub_km_state_t stub_target_km_get_state(void);
void stub_target_km_wait_for_state(stub_km_state_t state);

void stub_target_km_set_keygen_mode(stub_km_keygen_mode_t mode);
void stub_target_km_set_key_purpose(stub_km_key_purpose_t purpose);
void stub_target_km_use_sw_init_key(void);
/* @p use_256: false = XTS-AES-128, true = XTS-AES-256. Only meaningful for
 * XTS-AES key types; no-op for ECDSA / HMAC / DS. */
void stub_target_km_set_xts_aes_key_len(stub_km_key_type_t key_type, bool use_256);

void stub_target_km_start(void);
void stub_target_km_continue(void);

/* ---------- KM memory I/O (LOAD / GAIN phases) -------------------------- */

void stub_target_km_write_sw_init_key(const uint8_t *buf, size_t len);
void stub_target_km_write_assist_info(const uint8_t *buf, size_t len);
void stub_target_km_write_public_info(const uint8_t *buf, size_t len);
void stub_target_km_read_assist_info(uint8_t *buf, size_t len);
void stub_target_km_read_public_info(uint8_t *buf, size_t len);

/* ---------- Result checks ----------------------------------------------- */

/* Reads KEYMNG_HUK_VLD_REG. */
bool stub_target_km_is_huk_valid(void);

/* Reads KEYMNG_KEY_VLD_REG bit corresponding to the given (key_type, key_len).
 * The KEY_VLD encoding is split by ECDSA length — pass the same key_len
 * the deploy operation used. */
bool stub_target_km_is_key_deployment_valid(stub_km_key_type_t key_type, stub_km_key_len_t key_len);

/**
 * @brief Configure KEYMNG_USE_EFUSE_KEY for a given key type.
 *
 * After a successful deploy/recovery, the static register must direct the
 * peripheral to use the KM-deployed key (`use_own_key=true`) rather than
 * an eFuse-burned key. This is the very last step in every deploy / recover
 * sequence and matches `key_mgr_hal_set_key_usage(...USE_OWN_KEY)` in IDF.
 */
void stub_target_km_set_key_usage(stub_km_key_type_t key_type, bool use_own_key);

/**
 * @brief Check whether any eFuse key block has KM_INIT_KEY (purpose 12)
 * burned.
 *
 * Used by AES / ECDH1 deploy handlers before the chip is told to use the
 * eFuse init_key (USE_SW_INIT_KEY=0). If no key block has the right
 * purpose, the chip would silently decrypt k2_info with an all-zero key
 * and produce an invalid deployment — surfacing this as an explicit error
 * up-front saves debugging the resulting KEY_VLD=0 mystery.
 *
 * @return true if at least one block has purpose == KM_INIT_KEY; false
 *         otherwise. Implementations iterate over all 6 KEY block purpose
 *         fields; chip-specific because the field width and packing
 *         differ between targets (C5 uses 5-bit purposes, P4 4-bit).
 */
bool stub_target_km_is_efuse_init_key_burned(void);

#ifdef __cplusplus
}
#endif
