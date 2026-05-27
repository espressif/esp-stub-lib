/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * ESP32-P4 Key Manager peripheral registers (hw_ver3).
 *
 * Layout matches the ESP32-P4 register reference (hw_ver3) — the v3.0+
 * Key Manager has full parity with ESP32-C5: same offsets, same field
 * semantics. The first-generation KM on ESP32-P4 v1.x is disabled in
 * software (see SUPPORTS_KEY_MANAGER gating in esptool's esp32p4.py).
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "reg_base.h"

/* ---------- Configuration / control ------------------------------------- */

/** KEYMNG_STATIC_REG: KEY_LEN, USE_SW_INIT_KEY, USE_EFUSE_KEY etc. */
#define KEYMNG_STATIC_REG          (DR_REG_KEYMNG_BASE + 0x0018)
#define KEYMNG_USE_EFUSE_KEY_S     0U   /* bits[4:0]: 1 bit per key type */
#define KEYMNG_USE_EFUSE_KEY_M     0x1FU
#define KEYMNG_RND_SWITCH_CYCLE_S  5U   /* bits[9:5] */
#define KEYMNG_USE_SW_INIT_KEY_S   10U  /* bit10: 1=use sw_init_key, 0=use efuse */
#define KEYMNG_FLASH_KEY_LEN_S     11U  /* bit11: 0=XTS-AES-128, 1=XTS-AES-256 */
#define KEYMNG_PSRAM_KEY_LEN_S     12U

/** KEYMNG_LOCK_REG: write-1 lock for fields in STATIC_REG (one-shot). */
#define KEYMNG_LOCK_REG            (DR_REG_KEYMNG_BASE + 0x001C)

/** KEYMNG_CONF_REG: KGEN_MODE + KEY_PURPOSE. Configured in IDLE phase. */
#define KEYMNG_CONF_REG            (DR_REG_KEYMNG_BASE + 0x0020)
#define KEYMNG_KGEN_MODE_S         0U   /* bits[2:0]: deploy mode */
#define KEYMNG_KGEN_MODE_M         0x7U
#define KEYMNG_KEY_PURPOSE_S       3U   /* bits[6:3]: target key purpose 1..12 */
#define KEYMNG_KEY_PURPOSE_M       0xFU

/** KEYMNG_START_REG: write 1 to advance state machine. */
#define KEYMNG_START_REG           (DR_REG_KEYMNG_BASE + 0x0024)
#define KEYMNG_START               (1U << 0)
#define KEYMNG_CONTINUE            (1U << 1)

/** KEYMNG_STATE_REG: current FSM phase. */
#define KEYMNG_STATE_REG           (DR_REG_KEYMNG_BASE + 0x0028)
#define KEYMNG_STATE_M             0x3U
/* Values: 0=IDLE, 1=LOAD, 2=GAIN, 3=BUSY */

/** KEYMNG_RESULT_REG: per-bit per-key-type "deploy operation succeeded". */
#define KEYMNG_RESULT_REG          (DR_REG_KEYMNG_BASE + 0x002C)

/** KEYMNG_KEY_VLD_REG: per-bit "this (key_type, key_len) has been deployed
 *  and is ready to use". Note these bits are split by ECDSA *length* — not
 *  the same encoding as KEYMNG_USE_EFUSE_KEY (which is by type only).
 *  Layout matches ESP32-C5 register reference. */
#define KEYMNG_KEY_VLD_REG          (DR_REG_KEYMNG_BASE + 0x0030)
#define KEYMNG_KEY_VLD_ECDSA_192_M  (1U << 0)
#define KEYMNG_KEY_VLD_ECDSA_256_M  (1U << 1)
#define KEYMNG_KEY_VLD_FLASH_M      (1U << 2)
#define KEYMNG_KEY_VLD_HMAC_M       (1U << 3)
#define KEYMNG_KEY_VLD_DS_M         (1U << 4)
#define KEYMNG_KEY_VLD_PSRAM_M      (1U << 5)
#define KEYMNG_KEY_VLD_ECDSA_384_M  (1U << 6)

/** KEYMNG_HUK_VLD_REG: 1 if a HUK is currently loaded into the KM. */
#define KEYMNG_HUK_VLD_REG         (DR_REG_KEYMNG_BASE + 0x0034)
#define KEYMNG_HUK_VLD_M           (1U << 0)

/* ---------- Memory blocks ----------------------------------------------- */

/** Assist info: write k2_info / read k2*G (ECDH0) or write key_info (recovery). */
#define KEYMNG_ASSIST_INFO_MEM     (DR_REG_KEYMNG_BASE + 0x0100) /* 64 bytes */
#define KEYMNG_ASSIST_INFO_MEM_SIZE 64

/** Public info: write k1_encrypted (AES) or k1*G (ECDH0/1) or read key_info. */
#define KEYMNG_PUBLIC_INFO_MEM     (DR_REG_KEYMNG_BASE + 0x0140) /* 64 bytes */
#define KEYMNG_PUBLIC_INFO_MEM_SIZE 64

/** sw_init_key (32 bytes), used when KEYMNG_USE_SW_INIT_KEY=1. */
#define KEYMNG_SW_INIT_KEY_MEM     (DR_REG_KEYMNG_BASE + 0x0180)
#define KEYMNG_SW_INIT_KEY_MEM_SIZE 32

#ifdef __cplusplus
}
#endif
