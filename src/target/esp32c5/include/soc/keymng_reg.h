/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * ESP32-C5 Key Manager peripheral registers.
 *
 * Layout matches "ESP32-C5 Technical Reference Manual" Chapter 31 §31.11.2.
 * Field semantics follow §31.6.2 + §31.7.2; see comments inline.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "reg_base.h"

/* ---------- PCR registers used to bring the KM peripheral up ------------ */

/** PCR_SEC_CONF_REG: master crypto-subsystem clock select. C5 ROM/IDF
 *  default is XTAL on cold boot; IDF's esp_crypto_clk_init() switches
 *  it to 480 MHz SPLL (PCR_SEC_CLK_SEL=2) before any KM/HUK operation.
 *  Without that, the HUK Generator's PUF readout is unstable. */
#define PCR_SEC_CONF_REG       (DR_REG_PCR_BASE + 0x13C)
#define PCR_SEC_CLK_SEL_S      0U
#define PCR_SEC_CLK_SEL_M      0x3U
#define PCR_SEC_CLK_SEL_SPLL   0x2U  /* 480 MHz */

/** PCR_KM_CONF_REG: KM clock + reset + ready bits.
 *  After the reset pulse (set RST=1, then 0) the caller MUST poll for
 *  PCR_KM_READY before issuing any KM register access — otherwise the
 *  KM peripheral can latch register writes silently. The ROM's
 *  pcr_ll_km_en() does this; we now match it. */
#define PCR_KM_CONF_REG    (DR_REG_PCR_BASE + 0x164)
#define PCR_KM_CLK_EN_M    (1U << 0)
#define PCR_KM_RST_EN_M    (1U << 1)
#define PCR_KM_READY_M     (1U << 2)

/** PCR_KM_PD_CTRL_REG: KM memory power-down/up bits.
 *  Defaults to powered-down — must clear PD and set PU before any KM
 *  memory access (KEYMNG_*_MEM region) or reads return zero. */
#define PCR_KM_PD_CTRL_REG (DR_REG_PCR_BASE + 0x168)
#define PCR_KM_MEM_FORCE_PU_M (1U << 1)
#define PCR_KM_MEM_FORCE_PD_M (1U << 2)

/** PCR_ECC_CONF_REG: ECC peripheral clock + reset.  The KM uses the
 *  external ECC block to compute k1*G and k2*G during ECDH0 / ECDH1
 *  deploys — if the ECC clock isn't enabled the chip silently returns
 *  zero for k2*G even though KEYMNG_KEY_VLD ends up set. */
#define PCR_ECC_CONF_REG   (DR_REG_PCR_BASE + 0xDC)
#define PCR_ECC_CLK_EN_M   (1U << 0)
#define PCR_ECC_RST_EN_M   (1U << 1)

/** PCR_ECC_PD_CTRL_REG: ECC memory power-down/up bits.  Defaults are
 *  powered-down; clear both bits so the ECC peripheral's parameter memory
 *  is reachable when the KM drives it for ECDH0 / ECDH1. */
#define PCR_ECC_PD_CTRL_REG   (DR_REG_PCR_BASE + 0xE0)
#define PCR_ECC_MEM_PD_M       (1U << 0)
#define PCR_ECC_MEM_FORCE_PD_M (1U << 2)

/** PCR_ECDSA_CONF_REG: ECDSA peripheral reset.  The ECDSA block sits in
 *  series with the ECC multiplier; when its RST_EN bit is asserted (the
 *  ROM default) the ECC stays held in reset and KM-driven ECDH0 silently
 *  returns zero for k2*G.  Clearing this bit lets ECC come out of reset. */
#define PCR_ECDSA_CONF_REG (DR_REG_PCR_BASE + 0xEC)
#define PCR_ECDSA_RST_EN_M (1U << 1)

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
 *  the same encoding as KEYMNG_USE_EFUSE_KEY (which is by type only). */
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
