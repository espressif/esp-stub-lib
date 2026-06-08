/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * ESP32-P4 HP_SYS_CLKRST registers — only the subset the Key Manager HAL
 * needs (KM bus clock + peripheral clock + reset, parent crypto reset).
 *
 * Layout matches the ESP32-P4 register reference; field numbering follows
 * IDF's `soc/hp_sys_clkrst_reg.h`. ESP32-P4 uses these registers in place
 * of the PCR registers that ESP32-C5 has for the equivalent functions.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "reg_base.h"

/** HP_SYS_CLKRST_SOC_CLK_CTRL1_REG: SoC-level system clock gates.
 *  Bit 27 = KEY_MANAGER_SYS_CLK_EN — the KM bus clock. Defaults to enabled
 *  on cold boot but explicit re-assertion is the cheapest insurance against
 *  a previous boot leaving it gated off. */
#define HP_SYS_CLKRST_SOC_CLK_CTRL1_REG          (DR_REG_HP_SYS_CLKRST_BASE + 0x18)
#define HP_SYS_CLKRST_KEY_MANAGER_SYS_CLK_EN_M   (1U << 27)

/** HP_SYS_CLKRST_PERI_CLK_CTRL25_REG: peripheral clock gates for the
 *  crypto sub-block (ECC, ECDSA, KM all live here). The KM peripheral
 *  clock is bit 22 — independent of the bus clock above. */
#define HP_SYS_CLKRST_PERI_CLK_CTRL25_REG        (DR_REG_HP_SYS_CLKRST_BASE + 0xA8)
#define HP_SYS_CLKRST_CRYPTO_ECC_CLK_EN_M        (1U << 16)
#define HP_SYS_CLKRST_CRYPTO_ECDSA_CLK_EN_M      (1U << 21)
#define HP_SYS_CLKRST_CRYPTO_KM_CLK_EN_M         (1U << 22)

/** HP_SYS_CLKRST_HP_RST_EN2_REG: reset bits for HP-domain crypto blocks.
 *  RST_EN_CRYPTO (bit 14) is the parent crypto reset — when asserted it
 *  holds the KM in reset regardless of RST_EN_KM. Both bits must be 0 for
 *  the KM to operate. */
#define HP_SYS_CLKRST_HP_RST_EN2_REG             (DR_REG_HP_SYS_CLKRST_BASE + 0xC8)
#define HP_SYS_CLKRST_RST_EN_CRYPTO_M            (1U << 14)
#define HP_SYS_CLKRST_RST_EN_ECDSA_M             (1U << 20)
#define HP_SYS_CLKRST_RST_EN_ECC_M               (1U << 22)
#define HP_SYS_CLKRST_RST_EN_KM_M                (1U << 23)

#ifdef __cplusplus
}
#endif
