/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * ESP32-P4 Key Manager + HUK Generator HAL.
 *
 * Single-file port of the operations the stub keymanager plugin needs out
 * of ESP-IDF's `components/esp_security/src/esp_key_mgr.c`. The HAL/LL
 * split from IDF is collapsed (no portability layer needed in the stub).
 * OS locks, logging, eFuse helpers, and chip-clock plumbing from the IDF
 * version are dropped — the stub runs single-threaded.
 *
 * Compared with the ESP32-C5 sibling, ESP32-P4 uses HP_SYS_CLKRST instead
 * of PCR for clock/reset control and does not need the LP_AON PUF SRAM
 * power dance (SOC_HUK_MEM_NEEDS_RECHARGE=0). Field semantics and KEY_VLD
 * bit layout are identical to C5 on v3.0+ silicon.
 *
 * Reference: ESP-IDF `components/esp_hal_security/esp32p4/include/hal/`
 *   - key_mgr_ll.h  (bus clock + reset sequencing)
 *   - huk_ll.h      (HUK generator state machine)
 *   and the ESP32-P4 register reference (hw_ver3).
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <esp-stub-lib/soc_utils.h>

#include <target/key_mgr.h>

#include <soc/hp_sys_clkrst_reg.h>
#include <soc/huk_reg.h>
#include <soc/keymng_reg.h>

extern void *memcpy(void *dest, const void *src, size_t n);

/* ---------- Small utilities --------------------------------------------- */

/* Write @p len bytes to a 32-bit-aligned memory-mapped peripheral region.
 * Both ASSIST_INFO_MEM and PUBLIC_INFO_MEM only accept 4-byte stores; @p len
 * is rounded up to a 4-byte boundary internally. */
static void km_write_mem(uint32_t reg, const uint8_t *src, size_t len)
{
    uint32_t i = 0;
    while (i + 4 <= len) {
        uint32_t word = ((uint32_t)src[i])
                      | ((uint32_t)src[i + 1] << 8)
                      | ((uint32_t)src[i + 2] << 16)
                      | ((uint32_t)src[i + 3] << 24);
        REG_WRITE(reg + i, word);
        i += 4;
    }
    if (i < len) {
        uint32_t word = 0;
        for (size_t j = 0; j + i < len; j++) {
            word |= (uint32_t)src[i + j] << (8U * j);
        }
        REG_WRITE(reg + i, word);
    }
}

static void km_read_mem(uint32_t reg, uint8_t *dst, size_t len)
{
    uint32_t i = 0;
    while (i + 4 <= len) {
        uint32_t word = REG_READ(reg + i);
        dst[i + 0] = (uint8_t)(word & 0xFF);
        dst[i + 1] = (uint8_t)((word >> 8) & 0xFF);
        dst[i + 2] = (uint8_t)((word >> 16) & 0xFF);
        dst[i + 3] = (uint8_t)((word >> 24) & 0xFF);
        i += 4;
    }
    if (i < len) {
        uint32_t word = REG_READ(reg + i);
        for (size_t j = 0; j + i < len; j++) {
            dst[i + j] = (uint8_t)((word >> (8U * j)) & 0xFFU);
        }
    }
}

/* ---------- HUK Generator ----------------------------------------------- */

void stub_target_huk_get_status(uint8_t *gen_status, uint8_t *risk_level)
{
    uint32_t reg = REG_READ(HUK_STATUS_REG);
    if (gen_status) {
        *gen_status = (uint8_t)((reg >> HUK_STATUS_S) & HUK_STATUS_M);
    }
    if (risk_level) {
        *risk_level = (uint8_t)((reg >> HUK_RISK_LEVEL_S) & HUK_RISK_LEVEL_M);
    }
}

/* HUK_MODE values matching the ROM signature (huk_mode_t in rom/km.h). */
#define HUK_ROM_MODE_RECOVER 0
#define HUK_ROM_MODE_GEN     1

/* esp_rom_km_huk_conf — full HUK state-machine driver, including state
 * polling and (in GENERATE mode) the read-out into huk_info_buf. Returns
 * ETS_OK (=0) on success.
 *
 * Defined in P4 ROM (different address on rev1 vs eco5) — provided by
 * esp32p4.rom.ld / esp32p4.rom.eco5.ld. */
extern int esp_rom_km_huk_conf(int mode, uint8_t *huk_info);

int stub_target_huk_configure(stub_huk_mode_t mode, uint8_t *huk_info_buf)
{
    if (huk_info_buf == NULL) {
        return -1;
    }

    int rom_mode = (mode == STUB_HUK_MODE_GENERATE)
                       ? HUK_ROM_MODE_GEN
                       : HUK_ROM_MODE_RECOVER;

    /* P4 doesn't need the LP_AON PUF SRAM recharge that C5 requires on cold
     * boot — SOC_HUK_MEM_NEEDS_RECHARGE=0 in IDF's P4 soc_caps. The ROM
     * HUK configure routine sequences power-up and state polling on its own. */
    (void)esp_rom_km_huk_conf(rom_mode, huk_info_buf);

    uint8_t gen = 0;
    stub_target_huk_get_status(&gen, NULL);
    return (gen == 1U) ? 0 : -1;
}

/* ---------- Key Manager bring-up ---------------------------------------- */

void stub_target_km_bringup(void)
{
    /* Mirror IDF's esp_key_mgr_init (esp_security/src/init.c):
     *   1. Power up KM (no-op on P4 — empty key_mgr_ll_power_up)
     *   2. Enable bus clock (HP_SYS_CLKRST.soc_clk_ctrl1.reg_key_manager_sys_clk_en)
     *   3. Enable peripheral clock (HP_SYS_CLKRST.peri_clk_ctrl25.reg_crypto_km_clk_en)
     *   4. Reset KM (pulse RST_EN_KM)
     *   5. Clear parent crypto reset (RST_EN_CRYPTO) — without this the KM
     *      stays held in reset regardless of RST_EN_KM
     *   6. Wait for the KM state machine to fall through into IDLE
     *
     * Also enable ECC + ECDSA peripheral clocks and clear their resets so
     * ECDH0 / ECDH1 deploys can drive the external ECC multiplier without
     * silently producing zero for k2*G. The C5 sibling does an equivalent
     * sequence via PCR registers. */

    /* Bus clock + crypto peripheral clocks. */
    REG_SET_BIT(HP_SYS_CLKRST_SOC_CLK_CTRL1_REG,
                HP_SYS_CLKRST_KEY_MANAGER_SYS_CLK_EN_M);

    REG_SET_BIT(HP_SYS_CLKRST_PERI_CLK_CTRL25_REG,
                HP_SYS_CLKRST_CRYPTO_KM_CLK_EN_M
                | HP_SYS_CLKRST_CRYPTO_ECC_CLK_EN_M
                | HP_SYS_CLKRST_CRYPTO_ECDSA_CLK_EN_M);

    /* Reset pulse for KM, then leave reset clear. */
    REG_SET_BIT(HP_SYS_CLKRST_HP_RST_EN2_REG, HP_SYS_CLKRST_RST_EN_KM_M);
    REG_CLR_BIT(HP_SYS_CLKRST_HP_RST_EN2_REG, HP_SYS_CLKRST_RST_EN_KM_M);

    /* Clear parent crypto reset + ECC / ECDSA resets so the KM can drive
     * the external ECC block during ECDH0 / ECDH1 deploys. */
    REG_CLR_BIT(HP_SYS_CLKRST_HP_RST_EN2_REG,
                HP_SYS_CLKRST_RST_EN_CRYPTO_M
                | HP_SYS_CLKRST_RST_EN_ECC_M
                | HP_SYS_CLKRST_RST_EN_ECDSA_M);

    /* Wait for KM to settle into IDLE before any subsequent register
     * access. Reading state before this point can return non-IDLE
     * depending on what the previous boot left in the FSM. */
    while ((REG_READ(KEYMNG_STATE_REG) & KEYMNG_STATE_M) != 0U) {
        ;
    }

    /* Mirror IDF's esp_key_mgr_init final step: force USE_EFUSE_KEY for the
     * XTS-AES Flash key type on non-flash-encrypted boots. Without this the
     * KM is in a default state where it expects a deployed key for flash
     * decryption, which can interfere with subsequent HUK operations. The
     * HUK peripheral's own register/memory clock gates are configured by
     * the ROM huk_conf routine on demand — no separate setup needed here,
     * unlike C5 which has LP_AON power control to drive. */
    REG_SET_BIT(KEYMNG_STATIC_REG, 1U << 1);
}

/* ---------- Key Manager state machine ----------------------------------- */

stub_km_state_t stub_target_km_get_state(void)
{
    return (stub_km_state_t)(REG_READ(KEYMNG_STATE_REG) & KEYMNG_STATE_M);
}

void stub_target_km_wait_for_state(stub_km_state_t state)
{
    while (stub_target_km_get_state() != state) {
        ;
    }
}

void stub_target_km_set_keygen_mode(stub_km_keygen_mode_t mode)
{
    uint32_t reg = REG_READ(KEYMNG_CONF_REG);
    reg &= ~(KEYMNG_KGEN_MODE_M << KEYMNG_KGEN_MODE_S);
    reg |= ((uint32_t)mode & KEYMNG_KGEN_MODE_M) << KEYMNG_KGEN_MODE_S;
    REG_WRITE(KEYMNG_CONF_REG, reg);
}

void stub_target_km_set_key_purpose(stub_km_key_purpose_t purpose)
{
    uint32_t reg = REG_READ(KEYMNG_CONF_REG);
    reg &= ~(KEYMNG_KEY_PURPOSE_M << KEYMNG_KEY_PURPOSE_S);
    reg |= ((uint32_t)purpose & KEYMNG_KEY_PURPOSE_M) << KEYMNG_KEY_PURPOSE_S;
    REG_WRITE(KEYMNG_CONF_REG, reg);
}

void stub_target_km_use_sw_init_key(void)
{
    REG_SET_BIT(KEYMNG_STATIC_REG, 1U << KEYMNG_USE_SW_INIT_KEY_S);
}

void stub_target_km_set_xts_aes_key_len(stub_km_key_type_t key_type, bool use_256)
{
    uint32_t shift;
    if (key_type == STUB_KM_KEY_TYPE_FLASH_XTS_AES) {
        shift = KEYMNG_FLASH_KEY_LEN_S;
    } else if (key_type == STUB_KM_KEY_TYPE_PSRAM_XTS_AES) {
        shift = KEYMNG_PSRAM_KEY_LEN_S;
    } else {
        return;  /* not an XTS-AES key type — KM ignores the len bit */
    }
    if (use_256) {
        REG_SET_BIT(KEYMNG_STATIC_REG, 1U << shift);
    } else {
        REG_CLR_BIT(KEYMNG_STATIC_REG, 1U << shift);
    }
}

void stub_target_km_start(void)
{
    REG_WRITE(KEYMNG_START_REG, KEYMNG_START);
}

void stub_target_km_continue(void)
{
    REG_WRITE(KEYMNG_START_REG, KEYMNG_CONTINUE);
}

/* ---------- KM memory I/O ----------------------------------------------- */

void stub_target_km_write_sw_init_key(const uint8_t *buf, size_t len)
{
    km_write_mem(KEYMNG_SW_INIT_KEY_MEM, buf, len);
}

void stub_target_km_write_assist_info(const uint8_t *buf, size_t len)
{
    km_write_mem(KEYMNG_ASSIST_INFO_MEM, buf, len);
}

void stub_target_km_write_public_info(const uint8_t *buf, size_t len)
{
    km_write_mem(KEYMNG_PUBLIC_INFO_MEM, buf, len);
}

void stub_target_km_read_assist_info(uint8_t *buf, size_t len)
{
    km_read_mem(KEYMNG_ASSIST_INFO_MEM, buf, len);
}

void stub_target_km_read_public_info(uint8_t *buf, size_t len)
{
    km_read_mem(KEYMNG_PUBLIC_INFO_MEM, buf, len);
}

/* ---------- Result checks ----------------------------------------------- */

bool stub_target_km_is_huk_valid(void)
{
    return (REG_READ(KEYMNG_HUK_VLD_REG) & KEYMNG_HUK_VLD_M) != 0U;
}

/* USE_EFUSE_KEY is keyed by stub_km_key_type_t enum (0..4). KEY_VLD has a
 * different encoding split by ECDSA length — see key_vld_mask().
 * The shift form avoids producing a .rodata jump table. */
static uint32_t key_type_bit(stub_km_key_type_t key_type)
{
    if ((unsigned)key_type > 4U) {
        return 0U;
    }
    return 1U << (unsigned)key_type;
}

/* Per-bit lookup for KEYMNG_KEY_VLD — explicitly enumerated because the
 * length-aware split (ECDSA-192/256/384 occupy bits 0/1/6) doesn't map to
 * a single shift formula. */
static uint32_t key_vld_mask(stub_km_key_type_t kt, stub_km_key_len_t kl)
{
    if (kt == STUB_KM_KEY_TYPE_ECDSA) {
        if (kl == STUB_KM_KEY_LEN_ECDSA_192) { return KEYMNG_KEY_VLD_ECDSA_192_M; }
        if (kl == STUB_KM_KEY_LEN_ECDSA_256) { return KEYMNG_KEY_VLD_ECDSA_256_M; }
        if (kl == STUB_KM_KEY_LEN_ECDSA_384) { return KEYMNG_KEY_VLD_ECDSA_384_M; }
    }
    if (kt == STUB_KM_KEY_TYPE_FLASH_XTS_AES) { return KEYMNG_KEY_VLD_FLASH_M; }
    if (kt == STUB_KM_KEY_TYPE_HMAC)          { return KEYMNG_KEY_VLD_HMAC_M; }
    if (kt == STUB_KM_KEY_TYPE_DS)            { return KEYMNG_KEY_VLD_DS_M; }
    if (kt == STUB_KM_KEY_TYPE_PSRAM_XTS_AES) { return KEYMNG_KEY_VLD_PSRAM_M; }
    return 0U;
}

bool stub_target_km_is_key_deployment_valid(stub_km_key_type_t key_type,
                                            stub_km_key_len_t key_len)
{
    uint32_t mask = key_vld_mask(key_type, key_len);
    if (mask == 0U) {
        return false;
    }
    return (REG_READ(KEYMNG_KEY_VLD_REG) & mask) != 0U;
}

void stub_target_km_set_key_usage(stub_km_key_type_t key_type, bool use_own_key)
{
    /* KEYMNG_USE_EFUSE_KEY[bit_for_type] = 1 → use eFuse, 0 → use KM-deployed.
     * Each key type maps to a single bit, in the same order as KEYMNG_KEY_VLD_REG. */
    uint32_t mask = key_type_bit(key_type);
    if (mask == 0U) {
        return;
    }
    if (use_own_key) {
        REG_CLR_BIT(KEYMNG_STATIC_REG, mask);
    } else {
        REG_SET_BIT(KEYMNG_STATIC_REG, mask);
    }
}

/* ---------- eFuse: KM_INIT_KEY presence ---------------------------------- */

/* ESP32-P4 key purpose layout (esptool/targets/esp32p4.py):
 *   KEY0_PURPOSE: EFUSE_BASE + 0x34, bits[27:24]  (4-bit field)
 *   KEY1_PURPOSE: EFUSE_BASE + 0x34, bits[31:28]
 *   KEY2_PURPOSE: EFUSE_BASE + 0x38, bits[ 3: 0]
 *   KEY3_PURPOSE: EFUSE_BASE + 0x38, bits[ 7: 4]
 *   KEY4_PURPOSE: EFUSE_BASE + 0x38, bits[11: 8]
 *   KEY5_PURPOSE: EFUSE_BASE + 0x38, bits[15:12]
 * KM_INIT_KEY purpose value = 12 (matches IDF esp_efuse_chip.h on P4).
 *
 * The check unrolls the 6 slots inline — a slots[] table would land in
 * .rodata, which the plugin linker script forbids ("Plugin must not have
 * initialized .data — use BSS instead"). */
#define EFUSE_P4_KEY_PURPOSE_MASK         0xFU
#define EFUSE_P4_KM_INIT_KEY_PURPOSE_VAL  12U

bool stub_target_km_is_efuse_init_key_burned(void)
{
    uint32_t r34 = REG_READ(DR_REG_EFUSE_BASE + 0x34);
    uint32_t r38 = REG_READ(DR_REG_EFUSE_BASE + 0x38);
    const uint32_t mask   = EFUSE_P4_KEY_PURPOSE_MASK;
    const uint32_t target = EFUSE_P4_KM_INIT_KEY_PURPOSE_VAL;

    if (((r34 >> 24) & mask) == target) { return true; }  /* KEY0 */
    if (((r34 >> 28) & mask) == target) { return true; }  /* KEY1 */
    if (((r38 >>  0) & mask) == target) { return true; }  /* KEY2 */
    if (((r38 >>  4) & mask) == target) { return true; }  /* KEY3 */
    if (((r38 >>  8) & mask) == target) { return true; }  /* KEY4 */
    if (((r38 >> 12) & mask) == target) { return true; }  /* KEY5 */
    return false;
}
