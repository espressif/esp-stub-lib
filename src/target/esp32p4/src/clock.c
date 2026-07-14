/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <soc_utils.h>

#include <esp-stub-lib/rom_wrappers.h>

#include <target/clock.h>

#include <soc/efuse_reg.h>
#include <soc/hp_sys_clkrst_reg.h>
#include <soc/lp_clkrst_reg.h>
#include <soc/lp_system_reg.h>
#include <soc/lp_wdt_reg.h>
#include <soc/pmu_reg.h>
#include <soc/reg_base.h>
#include <soc/soc.h>

/* Match ESP-IDF pmu_param.h defaults used in rtc_clk_init(). */
#define HP_CALI_ACTIVE_DCM_VSET_DEFAULT 27
#define HP_CALI_ACTIVE_DBIAS_DEFAULT    24
#define LP_CALI_ACTIVE_DBIAS_DEFAULT    29

/* IDF rtc_clk_init() delay after DCDC enable / dbias handoff. */
#define DCDC_SETTLE_US                  1000

/* CPLL stays at ROM-calibrated 400 MHz; CPU uses integer /2 (no CPLL recall). */
#define CPU_FREQ_MHZ                    200

extern uint32_t esp_rom_get_cpu_freq(void);
extern void esp_rom_set_cpu_ticks_per_us(uint32_t ticks_per_us);

static uint32_t s_cpu_freq = 0;

static unsigned stub_target_get_chip_revision(void)
{
    /* P4 v3+: wafer version lives in EFUSE_RD_MAC_SYS_2 (same as IDF efuse_ll). */
    uint32_t reg = REG_READ(EFUSE_RD_MAC_SYS_2_REG);
    unsigned major_lo = (reg >> EFUSE_WAFER_VERSION_MAJOR_LO_S) & EFUSE_WAFER_VERSION_MAJOR_LO_V;
    unsigned major_hi = (reg >> EFUSE_WAFER_VERSION_MAJOR_HI_S) & 1U;
    unsigned major = (major_hi << 2) | major_lo;
    unsigned minor = (reg >> EFUSE_WAFER_VERSION_MINOR_S) & EFUSE_WAFER_VERSION_MINOR_V;

    return major * 100U + minor;
}

/* Match IDF efuse_hal_blk_version() / get_act_*_dbias() in pmu_param.c */
static unsigned stub_target_get_blk_version(void)
{
    uint32_t reg = REG_READ(EFUSE_RD_MAC_SYS_2_REG);
    unsigned major = (reg >> EFUSE_BLK_VERSION_MAJOR_S) & EFUSE_BLK_VERSION_MAJOR_V;
    unsigned minor = (reg >> EFUSE_BLK_VERSION_MINOR_S) & EFUSE_BLK_VERSION_MINOR_V;

    return major * 100U + minor;
}

static uint32_t stub_target_get_act_hp_dbias(void)
{
    uint32_t hp_cali_dbias = HP_CALI_ACTIVE_DBIAS_DEFAULT;
    unsigned blk_version = stub_target_get_blk_version();
    uint32_t hp_cali_dbias_efuse = 0;

    if (blk_version >= 2U && blk_version != 100U) {
        hp_cali_dbias_efuse = (REG_READ(EFUSE_RD_MAC_SYS_4_REG) >> EFUSE_ACTIVE_HP_DBIAS_S) & EFUSE_ACTIVE_HP_DBIAS_V;
    }
    if (hp_cali_dbias_efuse > 0U) {
        hp_cali_dbias = hp_cali_dbias_efuse + 16U;
        if (hp_cali_dbias > 31U) {
            hp_cali_dbias = 31U;
        }
    }
    return hp_cali_dbias;
}

static uint32_t stub_target_get_act_lp_dbias(void)
{
    uint32_t lp_cali_dbias = LP_CALI_ACTIVE_DBIAS_DEFAULT;
    unsigned blk_version = stub_target_get_blk_version();
    uint32_t lp_cali_dbias_efuse = 0;

    if (blk_version >= 2U && blk_version != 100U) {
        lp_cali_dbias_efuse = (REG_READ(EFUSE_RD_MAC_SYS_4_REG) >> EFUSE_ACTIVE_LP_DBIAS_S) & EFUSE_ACTIVE_LP_DBIAS_V;
    }
    if (lp_cali_dbias_efuse > 0U) {
        /* efuse dbias need to add 4 to near to dcdc voltage (IDF). */
        lp_cali_dbias = lp_cali_dbias_efuse + 16U + 4U;
        if (lp_cali_dbias > 31U) {
            lp_cali_dbias = 31U;
        }
    }
    return lp_cali_dbias;
}

static void stub_target_bus_update(void)
{
    SET_PERI_REG_MASK(HP_SYS_CLKRST_ROOT_CLK_CTRL0_REG, HP_SYS_CLKRST_REG_SOC_CLK_DIV_UPDATE);
    while (READ_PERI_REG(HP_SYS_CLKRST_ROOT_CLK_CTRL0_REG) & HP_SYS_CLKRST_REG_SOC_CLK_DIV_UPDATE) {
    }
}

static void stub_target_switch_to_dcdc(void)
{
    unsigned chip_version = stub_target_get_chip_revision();
    uint32_t hp_dbias = stub_target_get_act_hp_dbias();
    uint32_t lp_dbias = stub_target_get_act_lp_dbias();
    /*
     * IDF may raise this from PMU_HP_DBIAS_VOL when PVT is enabled. PVT is disabled
     * in the stub because that flow is not stable enough for ROM-download use and
     * may keep changing in IDF, so the default DCDC setting is sufficient here.
     */
    uint32_t hp_dcmvset = HP_CALI_ACTIVE_DCM_VSET_DEFAULT;

    SET_PERI_REG_MASK(PMU_HP_ACTIVE_HP_REGULATOR0_REG, PMU_HP_ACTIVE_HP_REGULATOR_XPD);
    REG_SET_FIELD(PMU_HP_ACTIVE_HP_REGULATOR0_REG, PMU_HP_ACTIVE_HP_REGULATOR_DBIAS, hp_dbias);
    REG_SET_FIELD(PMU_LP_ACTIVE_LP_REGULATOR0_REG, PMU_LP_ACTIVE_LP_REGULATOR_DBIAS, lp_dbias);

    /* ESP_CHIP_REV_ABOVE(rev, 301): ECO6+ FIB handoff for DCDC feedback. */
    if (chip_version >= 301U) {
        SET_PERI_REG_MASK(PMU_DCM_CTRL_REG, PMU_DCDC_FB_RES_FORCE_PD);
    }

    CLEAR_PERI_REG_MASK(PMU_DCM_CTRL_REG, PMU_DCDC_DONE_FORCE);
    SET_PERI_REG_MASK(PMU_DCM_CTRL_REG, PMU_DCDC_ON_REQ);
    /* pmu_ll_set_dcdc_switch_force_power_down(false) */
    CLEAR_PERI_REG_MASK(PMU_POWER_DCDC_SWITCH_REG, PMU_FORCE_DCDC_SWITCH_PU);
    CLEAR_PERI_REG_MASK(PMU_POWER_DCDC_SWITCH_REG, PMU_FORCE_DCDC_SWITCH_PD);
    REG_SET_FIELD(PMU_HP_ACTIVE_BIAS_REG, PMU_HP_ACTIVE_DCM_VSET, hp_dcmvset);
    SET_PERI_REG_MASK(PMU_HP_ACTIVE_HP_REGULATOR0_REG, PMU_DIG_REGULATOR0_DBIAS_SEL);
    stub_lib_delay_us(DCDC_SETTLE_US);

    if (chip_version >= 301U) {
        /* lp_fib_sel bit4=0 selects dig_fib_reg instead of ana_fib_reg. */
        REG_SET_FIELD(LP_SYSTEM_REG_SYS_CTRL_REG, LP_SYSTEM_REG_LP_FIB_SEL, 0xEF);
        CLEAR_PERI_REG_MASK(PMU_DCM_CTRL_REG, PMU_DCDC_FB_RES_FORCE_PD);
        stub_lib_delay_us(10);
    }

    /* Same as pmu_ll_hp_set_regulator_xpd(..., false) in rtc_clk_init(). */
    CLEAR_PERI_REG_MASK(PMU_HP_ACTIVE_HP_REGULATOR0_REG, PMU_HP_ACTIVE_HP_REGULATOR_XPD);
}

static void stub_target_apply_cpu_cpll_div2(void)
{
    uint32_t ctrl0;

    /*
     * Upscale from ROM download ~100 MHz (CPLL/4) to 200 MHz (CPLL/2).
     * Match IDF rtc_clk_cpu_freq_to_cpll_mhz(200) upscale order:
     * APB -> SYS -> MEM -> CPU. Do not retouch LP_CLKRST_HP_ROOT_CLK_SRC_SEL
     * or recalibrate CPLL — ROM already selected CPLL @ 400 MHz.
     *
     * Divider register fields store (divider - 1), same as clk_ll_*_set_divider().
     */
    REG_SET_FIELD(HP_SYS_CLKRST_ROOT_CLK_CTRL2_REG, HP_SYS_CLKRST_REG_APB_CLK_DIV_NUM, 1);
    stub_target_bus_update();
    /* sys_divider = 1 => div_num field 0 (IDF keeps SYS divider unchanged). */
    REG_SET_FIELD(HP_SYS_CLKRST_ROOT_CLK_CTRL1_REG, HP_SYS_CLKRST_REG_SYS_CLK_DIV_NUM, 0);
    stub_target_bus_update();
    REG_SET_FIELD(HP_SYS_CLKRST_ROOT_CLK_CTRL1_REG, HP_SYS_CLKRST_REG_MEM_CLK_DIV_NUM, 0);
    stub_target_bus_update();

    /* 400 / 2 = 200 */
    ctrl0 = READ_PERI_REG(HP_SYS_CLKRST_ROOT_CLK_CTRL0_REG);
    ctrl0 &= ~(HP_SYS_CLKRST_REG_CPU_CLK_DIV_NUM_M | HP_SYS_CLKRST_REG_CPU_CLK_DIV_NUMERATOR_M |
               HP_SYS_CLKRST_REG_CPU_CLK_DIV_DENOMINATOR_M);
    ctrl0 |= (1U << HP_SYS_CLKRST_REG_CPU_CLK_DIV_NUM_S);
    WRITE_PERI_REG(HP_SYS_CLKRST_ROOT_CLK_CTRL0_REG, ctrl0);
    stub_target_bus_update();
}

void stub_target_clock_init(void)
{
    stub_target_switch_to_dcdc();

    /* Publish ticks before divider change so ROM delays stay coherent. */
    s_cpu_freq = CPU_FREQ_MHZ * MHZ;
    esp_rom_set_cpu_ticks_per_us(CPU_FREQ_MHZ);
    stub_target_apply_cpu_cpll_div2();
}

uint32_t stub_target_get_cpu_freq(void)
{
    if (s_cpu_freq == 0) {
        return esp_rom_get_cpu_freq();
    }
    return s_cpu_freq;
}

#define LP_WDT_WDT_KEY 0x50D83AA1
#define LP_WDT_SWD_KEY 0x50D83AA1

void stub_target_clock_disable_watchdogs(void)
{
    // Disable RWDT (RTC Watchdog)
    REG_SET_BIT(LP_WDT_INT_CLR_REG, LP_WDT_LP_WDT_INT_CLR);
    WRITE_PERI_REG(LP_WDT_WPROTECT_REG, LP_WDT_WDT_KEY);
    WRITE_PERI_REG(LP_WDT_CONFIG0_REG, 0x0);
    WRITE_PERI_REG(LP_WDT_WPROTECT_REG, 0x0);

    // Configure SWD (Super Watchdog) to autofeed
    REG_SET_BIT(LP_WDT_INT_CLR_REG, LP_WDT_SUPER_WDT_INT_CLR);
    WRITE_PERI_REG(LP_WDT_SWD_WPROTECT_REG, LP_WDT_SWD_KEY);
    SET_PERI_REG_MASK(LP_WDT_SWD_CONFIG_REG, LP_WDT_SWD_AUTO_FEED_EN);
    WRITE_PERI_REG(LP_WDT_SWD_WPROTECT_REG, 0x0);
}
