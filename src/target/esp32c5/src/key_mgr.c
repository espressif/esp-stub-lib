/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * ESP32-C5 Key Manager + HUK Generator HAL.
 *
 * Single-file port of the operations the stub keymanager plugin needs out
 * of ESP-IDF's `components/esp_security/src/esp_key_mgr.c`. The HAL/LL
 * split from IDF is collapsed (no portability layer needed in the stub).
 * OS locks, logging, eFuse helpers, and chip-clock plumbing from the IDF
 * version are dropped — the stub runs single-threaded and arrives with
 * the necessary clocks already enabled by the ROM.
 *
 * Reference: ESP32-C5 Technical Reference Manual chapter 31.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <err.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/log.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/key_mgr.h>

#include <soc/efuse_reg.h>
#include <soc/huk_reg.h>
#include <soc/keymng_reg.h>

extern void *memcpy(void *dest, const void *src, size_t n);

/* ---------- Small utilities --------------------------------------------- */

/* Write @p len bytes to a 32-bit-aligned memory-mapped peripheral region.
 * Both ASSIST_INFO_MEM and PUBLIC_INFO_MEM only accept 4-byte stores; @p len
 * is rounded up to a 4-byte boundary internally. */
static void km_write_mem(uint32_t reg, const uint8_t *src, size_t len)
{
    /* Pad src to a 4-byte multiple so we never read past the caller's buffer
     * and can use plain word stores. The KM ignores extra zero bytes per
     * the IDF reference impl, but we must not read OOB on src. */
    uint32_t i = 0;
    while (i + 4 <= len) {
        uint32_t word = ((uint32_t)src[i]) | ((uint32_t)src[i + 1] << 8) | ((uint32_t)src[i + 2] << 16) |
                        ((uint32_t)src[i + 3] << 24);
        REG_WRITE(reg + i, word);
        i += 4;
    }
    if (i < len) {
        /* Tail: assemble a partial word with zero-padded high bytes. */
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

/* esp_rom_km_huk_conf — full HUK state-machine driver, including power-up
 * sequencing, state polling, and (in GENERATE mode) the read-out into
 * huk_info_buf. Returns ETS_OK (=0) on success.
 *
 * Defined in C5 ROM at 0x40000894 — provided by esp32c5.rom.ld. We only
 * need the prototype here; we don't pull in the IDF rom/km.h header
 * because it transitively requires soc/soc.h and ets_sys.h. */
extern int esp_rom_km_huk_conf(int mode, uint8_t *huk_info);

/* ROM delay helper, provided by esp32c5.rom.api.ld as esp_rom_delay_us. */
extern void esp_rom_delay_us(uint32_t us);

/* Power-cycle the PUF SRAM. C5 has SOC_HUK_MEM_NEEDS_RECHARGE=1: on cold
 * boot the PUF SRAM holds stale state, and a fresh huk_configure() will
 * report HUK valid in HUK_STATUS_REG but the KM will not pick it up
 * (KEYMNG_HUK_VLD stays 0). The recharge pulse re-initialises the SRAM
 * cells so a follow-up configure succeeds. */
static void huk_recharge_puf_memory(void)
{
    REG_CLR_BIT(LP_AON_MEM_CTRL_REG, LP_AON_HUK_MEM_FORCE_PD);
    REG_CLR_BIT(LP_AON_PUF_MEM_SW_REG, LP_AON_PUF_MEM_SW);
    REG_SET_BIT(LP_AON_PUF_MEM_DISCHARGE_REG, LP_AON_PUF_MEM_DISCHARGE);
    esp_rom_delay_us(100000U);
    REG_CLR_BIT(LP_AON_PUF_MEM_DISCHARGE_REG, LP_AON_PUF_MEM_DISCHARGE);
    REG_SET_BIT(LP_AON_PUF_MEM_SW_REG, LP_AON_PUF_MEM_SW);
    esp_rom_delay_us(100000U);
}

int stub_target_huk_configure(stub_huk_mode_t mode, uint8_t *huk_info_buf)
{
    if (huk_info_buf == NULL) {
        STUB_LOGE("huk_info_buf is NULL\n");
        return STUB_LIB_ERR_INVALID_ARG;
    }
    /* Clear FORCE_PD and assert FORCE_PU.  Clearing PD alone is enough for
     * the HUK Generator's own readout, but the KM peripheral's view of HUK
     * (KEYMNG_HUK_VLD latched as a side-effect of the next key-deploy) needs
     * the PUF SRAM forced on.  Matches A1's huk_power_up sequence which was
     * verified end-to-end on this C5 v1.2 board. */
    {
        uint32_t v = REG_READ(LP_AON_MEM_CTRL_REG);
        v &= ~LP_AON_HUK_MEM_FORCE_PD;
        v |= LP_AON_HUK_MEM_FORCE_PU;
        REG_WRITE(LP_AON_MEM_CTRL_REG, v);
    }

    int rom_mode = (mode == STUB_HUK_MODE_GENERATE) ? HUK_ROM_MODE_GEN : HUK_ROM_MODE_RECOVER;

    /* IDF reference path (configure_huk in esp_security/src/esp_key_mgr.c):
     * configure once; if the KM doesn't see HUK valid, recharge the PUF
     * SRAM and configure again. esp_rom_km_huk_conf always returns 0
     * per the ROM source (rom/key_mgr/key_mgr.c) — its return value
     * carries no information, so we ignore it.
     *
     * For same-session deploy → recover paths the success signal we use is
     * HUK_STATUS_REG.gen_status==1: that's what the HUK Generator reports
     * after GENERATE, and what same-session RECOVER preserves. IDF's
     * configure_huk asserts KEYMNG_HUK_VLD instead, but on this C5 v1.2
     * board the KM doesn't latch HUK from a standalone huk_configure —
     * it only sets KEYMNG_HUK_VLD as a side-effect of a subsequent
     * key-deploy. Cross-boot RECOVER on a cold chip currently fails
     * regardless of which signal we check (HUK_STATUS goes to gen=2/risk=7
     * even after recharge + retry); see the commit message for the test
     * matrix. */
    (void)esp_rom_km_huk_conf(rom_mode, huk_info_buf);

    uint8_t gen = 0;
    stub_target_huk_get_status(&gen, NULL);
    if (gen != 1U) {
        huk_recharge_puf_memory();
        (void)esp_rom_km_huk_conf(rom_mode, huk_info_buf);
        stub_target_huk_get_status(&gen, NULL);
    }
    if (gen != 1U) {
        STUB_LOGE("HUK Generator did not report gen_status=1 (got %u)\n", (unsigned)gen);
        return STUB_LIB_FAIL;
    }
    return STUB_LIB_OK;
}

/* ---------- Key Manager bring-up ---------------------------------------- */

void stub_target_km_bringup(void)
{
    /* Step 0: master crypto clock select. IDF's esp_crypto_clk_init runs
     * this at startup (esp_security/src/init.c, priority 103) before any
     * KM/HUK operation. The default after reset is XTAL — operations
     * sometimes succeed on XTAL but the HUK Generator's PUF readout is
     * unstable, manifesting as cross-boot RECOVER returning gen=2/risk=7. */
    {
        uint32_t v = REG_READ(PCR_SEC_CONF_REG);
        v &= ~(PCR_SEC_CLK_SEL_M << PCR_SEC_CLK_SEL_S);
        v |= (PCR_SEC_CLK_SEL_SPLL & PCR_SEC_CLK_SEL_M) << PCR_SEC_CLK_SEL_S;
        REG_WRITE(PCR_SEC_CONF_REG, v);
    }

    /* Mirror C5 ROM `pcr_ll_km_en()` exactly. Order matters: the ROM
     * source (rom/key_mgr/key_mgr.c → pcr_ll_km_en) does:
     *   1. Clear KM memory FORCE_PD
     *   2. Clear LP_AON HUK memory FORCE_PD (yes, KM bring-up touches HUK power)
     *   3. KM bus clock on
     *   4. Reset pulse (RST=1, then 0)
     *   5. Poll PCR_KM_READY */
    REG_CLR_BIT(PCR_KM_PD_CTRL_REG, PCR_KM_MEM_FORCE_PD_M);
    REG_CLR_BIT(LP_AON_MEM_CTRL_REG, LP_AON_HUK_MEM_FORCE_PD);

    REG_SET_BIT(PCR_KM_CONF_REG, PCR_KM_CLK_EN_M);
    REG_SET_BIT(PCR_KM_CONF_REG, PCR_KM_RST_EN_M);
    REG_CLR_BIT(PCR_KM_CONF_REG, PCR_KM_RST_EN_M);

    while ((REG_READ(PCR_KM_CONF_REG) & PCR_KM_READY_M) == 0U) {
        ;
    }

    /* Wait for the KM state machine to fall through into IDLE before any
     * subsequent register access — matches IDF's esp_key_mgr_init.
     * Reading state before this point can return non-IDLE depending on
     * what the previous boot left in the FSM. */
    while ((REG_READ(KEYMNG_STATE_REG) & KEYMNG_STATE_M) != 0U) {
        ;
    }

    /* The crucial cold-boot step IDF's esp_key_mgr_init() does for
     * non-flash-encrypted boots: set USE_EFUSE_KEY for the XTS-AES Flash
     * key type. Without this the KM is in a default state where it
     * expects a deployed key for flash decryption, which is never going
     * to materialise on a chip without flash encryption — and that
     * unresolved expectation interferes with subsequent HUK operations,
     * showing up empirically as cross-boot HUK recovery returning
     * gen=2/risk=7. The bit is at KEYMNG_USE_EFUSE_KEY[1] for FLASH. */
    REG_SET_BIT(KEYMNG_STATIC_REG, BIT(1));

    REG_SET_BIT(PCR_KM_PD_CTRL_REG, PCR_KM_MEM_FORCE_PU_M);

    /* Bring up the external ECC peripheral. The KM internally drives this
     * block for ECDH0 / ECDH1 scalar multiplications (k2*G generation).
     * Two register-bit fixes are needed; without either the chip silently
     * returns zero for k2*G even though KEYMNG_KEY_VLD ends up set:
     *   1. PCR_ECC_CONF.CLK_EN — bus clock on, plus a reset pulse.
     *   2. PCR_ECDSA_CONF.RST_EN — clear it.  The ECDSA block sits in series
     *      with ECC and stays in reset by default; while it's held the ECC
     *      multiplier doesn't run.  Mirrors A1's ecc_reset() sequence. */
    /* ECC memory must be powered before any ECC register access. */
    REG_CLR_BIT(PCR_ECC_PD_CTRL_REG, PCR_ECC_MEM_PD_M);
    REG_CLR_BIT(PCR_ECC_PD_CTRL_REG, PCR_ECC_MEM_FORCE_PD_M);
    REG_SET_BIT(PCR_ECC_CONF_REG, PCR_ECC_CLK_EN_M);
    REG_SET_BIT(PCR_ECC_CONF_REG, PCR_ECC_RST_EN_M);
    REG_CLR_BIT(PCR_ECC_CONF_REG, PCR_ECC_RST_EN_M);
    REG_CLR_BIT(PCR_ECDSA_CONF_REG, PCR_ECDSA_RST_EN_M);
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
    REG_SET_BIT(KEYMNG_STATIC_REG, BIT(KEYMNG_USE_SW_INIT_KEY_S));
}

void stub_target_km_set_xts_aes_key_len(stub_km_key_type_t key_type, bool use_256)
{
    uint32_t shift;
    if (key_type == STUB_KM_KEY_TYPE_FLASH_XTS_AES) {
        shift = KEYMNG_FLASH_KEY_LEN_S;
    } else if (key_type == STUB_KM_KEY_TYPE_PSRAM_XTS_AES) {
        shift = KEYMNG_PSRAM_KEY_LEN_S;
    } else {
        return; /* not an XTS-AES key type — KM ignores the len bit */
    }
    if (use_256) {
        REG_SET_BIT(KEYMNG_STATIC_REG, BIT(shift));
    } else {
        REG_CLR_BIT(KEYMNG_STATIC_REG, BIT(shift));
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
    if (buf == NULL) {
        STUB_LOGE("write_sw_init_key: NULL buf\n");
        return;
    }
    km_write_mem(KEYMNG_SW_INIT_KEY_MEM, buf, len);
}

void stub_target_km_write_assist_info(const uint8_t *buf, size_t len)
{
    if (buf == NULL) {
        STUB_LOGE("write_assist_info: NULL buf\n");
        return;
    }
    km_write_mem(KEYMNG_ASSIST_INFO_MEM, buf, len);
}

void stub_target_km_write_public_info(const uint8_t *buf, size_t len)
{
    if (buf == NULL) {
        STUB_LOGE("write_public_info: NULL buf\n");
        return;
    }
    km_write_mem(KEYMNG_PUBLIC_INFO_MEM, buf, len);
}

void stub_target_km_read_assist_info(uint8_t *buf, size_t len)
{
    if (buf == NULL) {
        STUB_LOGE("read_assist_info: NULL buf\n");
        return;
    }
    km_read_mem(KEYMNG_ASSIST_INFO_MEM, buf, len);
}

void stub_target_km_read_public_info(uint8_t *buf, size_t len)
{
    if (buf == NULL) {
        STUB_LOGE("read_public_info: NULL buf\n");
        return;
    }
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
    return BIT((unsigned)key_type);
}

/* Per-bit lookup for KEYMNG_KEY_VLD — explicitly enumerated because the
 * length-aware split (ECDSA-192/256/384 occupy bits 0/1/6) doesn't map to
 * a single shift formula. */
static uint32_t key_vld_mask(stub_km_key_type_t kt, stub_km_key_len_t kl)
{
    if (kt == STUB_KM_KEY_TYPE_ECDSA) {
        if (kl == STUB_KM_KEY_LEN_ECDSA_192) {
            return KEYMNG_KEY_VLD_ECDSA_192_M;
        }
        if (kl == STUB_KM_KEY_LEN_ECDSA_256) {
            return KEYMNG_KEY_VLD_ECDSA_256_M;
        }
        if (kl == STUB_KM_KEY_LEN_ECDSA_384) {
            return KEYMNG_KEY_VLD_ECDSA_384_M;
        }
    }
    if (kt == STUB_KM_KEY_TYPE_FLASH_XTS_AES) {
        return KEYMNG_KEY_VLD_FLASH_M;
    }
    if (kt == STUB_KM_KEY_TYPE_HMAC) {
        return KEYMNG_KEY_VLD_HMAC_M;
    }
    if (kt == STUB_KM_KEY_TYPE_DS) {
        return KEYMNG_KEY_VLD_DS_M;
    }
    if (kt == STUB_KM_KEY_TYPE_PSRAM_XTS_AES) {
        return KEYMNG_KEY_VLD_PSRAM_M;
    }
    return 0U;
}

bool stub_target_km_is_key_deployment_valid(stub_km_key_type_t key_type, stub_km_key_len_t key_len)
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

/* Inline-unrolled to keep the slot table out of .rodata (plugin linker
 * script forbids initialized .data). */
#define EFUSE_C5_KM_INIT_KEY_PURPOSE_VAL 12U

bool stub_target_km_is_efuse_init_key_burned(void)
{
    uint32_t data1 = REG_READ(EFUSE_RD_REPEAT_DATA1_REG);
    uint32_t data2 = REG_READ(EFUSE_RD_REPEAT_DATA2_REG);
    const uint32_t mask = EFUSE_C5_KEY_PURPOSE_M;
    const uint32_t target = EFUSE_C5_KM_INIT_KEY_PURPOSE_VAL;

    if (((data1 >> EFUSE_C5_KEY0_PURPOSE_IN_DATA1_S) & mask) == target) {
        return true;
    }
    if (((data1 >> EFUSE_C5_KEY1_PURPOSE_IN_DATA1_S) & mask) == target) {
        return true;
    }
    if (((data2 >> EFUSE_C5_KEY2_PURPOSE_IN_DATA2_S) & mask) == target) {
        return true;
    }
    if (((data2 >> EFUSE_C5_KEY3_PURPOSE_IN_DATA2_S) & mask) == target) {
        return true;
    }
    if (((data2 >> EFUSE_C5_KEY4_PURPOSE_IN_DATA2_S) & mask) == target) {
        return true;
    }
    if (((data2 >> EFUSE_C5_KEY5_PURPOSE_IN_DATA2_S) & mask) == target) {
        return true;
    }
    return false;
}
