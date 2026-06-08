/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * ESP32-P4 HUK Generator peripheral registers (hw_ver3).
 *
 * Layout matches the ESP32-P4 register reference (hw_ver3). Compared with
 * ESP32-C5 this file omits the LP_AON PUF SRAM power/discharge bits — the
 * P4 HUK Generator's MMIO is in the LP-AON peripheral region but doesn't
 * require the cold-boot PUF SRAM recharge dance that C5 needs
 * (SOC_HUK_MEM_NEEDS_RECHARGE=0 in IDF's P4 soc_caps).
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "reg_base.h"

/** HUK_CLK_REG: register and memory clock gates.
 *  HUK_CLK_EN (bit 0) defaults on — register-side clock.
 *  HUK_MEM_CG_FORCE_ON (bit 1) defaults OFF — without it HUK_INFO_MEM
 *  reads return zero even after a successful GENERATE phase. */
#define HUK_CLK_REG           (DR_REG_HUK_BASE + 0x0004)
#define HUK_CLK_EN_M          (1U << 0)
#define HUK_MEM_CG_FORCE_ON_M (1U << 1)

/** HUK_CONF_REG: HUK_MODE (bit 0). 0 = Recovery, 1 = Generation. */
#define HUK_CONF_REG          (DR_REG_HUK_BASE + 0x0020)
#define HUK_MODE_M            (1U << 0)

/** HUK_START_REG: write 1 to advance the state machine. */
#define HUK_START_REG         (DR_REG_HUK_BASE + 0x0024)
#define HUK_START             (1U << 0)
#define HUK_CONTINUE          (1U << 1)

/** HUK_STATE_REG: 0=IDLE, 1=LOAD, 2=GAIN, 3=BUSY. */
#define HUK_STATE_REG         (DR_REG_HUK_BASE + 0x0028)
#define HUK_STATE_M           0x3U

/** HUK_STATUS_REG: HUK_STATUS[1:0] + HUK_RISK_LEVEL[4:2]. */
#define HUK_STATUS_REG        (DR_REG_HUK_BASE + 0x0034)
#define HUK_STATUS_S          0U
#define HUK_STATUS_M          0x3U
#define HUK_RISK_LEVEL_S      2U
#define HUK_RISK_LEVEL_M      0x7U

/** HUK_INFO_MEM: 384-byte MMIO window — NOT the full huk_info. The ROM
 *  routine esp_rom_km_huk_conf iterates this window across multiple passes to
 *  read/write the full STUB_KM_HUK_INFO_SIZE (660) byte huk_info blob during
 *  generation (read out) and recovery (write in). */
#define HUK_INFO_MEM          (DR_REG_HUK_BASE + 0x0100)
#define HUK_INFO_MEM_SIZE     384

#ifdef __cplusplus
}
#endif
