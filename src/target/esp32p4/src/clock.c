/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>

#include <soc_utils.h>

#include <esp-stub-lib/rom_wrappers.h>

#include <target/clock.h>

#include <soc/efuse_reg.h>
#include <soc/hp_sys_clkrst_reg.h>
#include <soc/lp_clkrst_reg.h>
#include <soc/lp_i2c_ana_mst_reg.h>
#include <soc/lp_system_reg.h>
#include <soc/lp_wdt_reg.h>
#include <soc/lpperi_reg.h>
#include <soc/pmu_reg.h>
#include <soc/pvt_reg.h>
#include <soc/reg_base.h>
#include <soc/regi2c_cpll.h>
#include <soc/soc.h>


/* Match ESP-IDF pmu_param.h defaults used in rtc_clk_init(). */
#define HP_CALI_ACTIVE_DCM_VSET_DEFAULT 27
#define HP_CALI_ACTIVE_DBIAS_DEFAULT    24
#define LP_CALI_ACTIVE_DBIAS_DEFAULT    29

/* IDF rtc_clk_init() delay after DCDC enable / dbias handoff. */
#define DCDC_SETTLE_US                  1000

/* IDF pmu_init() delay after pvt_func_enable(true). */
#define PVT_SETTLE_US                   1000

/* Match IDF default CPU config for rev >= v3: CPLL 400 MHz, CPU /1. */
#define XTAL_FREQ_MHZ                   40
#define CPLL_FREQ_MHZ                   400
#define CPU_FREQ_MHZ                    400

/* Match ESP-IDF rtc.h PVT constants (CONFIG_ESP_ENABLE_PVT, rev >= v3). */
#define PVT_CHANNEL0_SEL    49
#define PVT_CHANNEL1_SEL    53
#define PVT_CHANNEL0_CFG    0x11fff
#define PVT_CHANNEL1_CFG    0x17fff
#define PVT_CHANNEL2_CFG    0x10000
#define PVT_CMD0            0x24
#define PVT_CMD1            0x5
#define PVT_CMD2            0x427
#define PVT_TARGET          0xffff
#define PVT_CLK_DIV         1
#define PVT_EDG_MODE        1
#define PVT_DELAY_NUM_HIGH  160
#define PVT_DELAY_NUM_LOW   153

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

/* Match IDF efuse_ll_get_dbias_vol_gap(). */
static int32_t stub_target_get_dbias_vol_gap(void)
{
    return (int32_t)((REG_READ(EFUSE_RD_MAC_SYS_5_REG) >> EFUSE_LP_DCDC_DBIAS_VOL_GAP_S) &
                     EFUSE_LP_DCDC_DBIAS_VOL_GAP_V);
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

/*
 * Ported from ESP-IDF components/esp_hw_support/port/esp32p4/pmu_pvt.c
 * Keep as separate functions; do not inline into stub_target_clock_init().
 */
static uint8_t get_lp_hp_gap(void)
{
    int8_t lp_hp_gap = 0;
    uint32_t blk_version = stub_target_get_blk_version();
    uint8_t lp_hp_gap_efuse = 0;
    if (blk_version >= 2 && blk_version != 100) {
        lp_hp_gap_efuse = (uint8_t)stub_target_get_dbias_vol_gap();
        bool gap_flag = lp_hp_gap_efuse >> 4;
        uint8_t gap_abs_value = lp_hp_gap_efuse & 0xf;
        if (gap_flag) {
            lp_hp_gap = (int8_t)(-1 * (int)gap_abs_value);
        } else {
            lp_hp_gap = (int8_t)gap_abs_value;
        }
        lp_hp_gap = (int8_t)(lp_hp_gap - 8);
        if (lp_hp_gap < 0) {
            lp_hp_gap = (int8_t)(16 - lp_hp_gap);
        }
    }
    return (uint8_t)lp_hp_gap;
}

static void set_pvt_hp_lp_gap(uint8_t value)
{
    bool flag = value >> 4;
    uint8_t abs_value = value & 0xf;

    SET_PERI_REG_BITS(PVT_DBIAS_CMD0_REG, PVT_DBIAS_CMD0_OFFSET_FLAG, flag, PVT_DBIAS_CMD0_OFFSET_FLAG_S);
    SET_PERI_REG_BITS(PVT_DBIAS_CMD0_REG, PVT_DBIAS_CMD0_OFFSET_VALUE, abs_value, PVT_DBIAS_CMD0_OFFSET_VALUE_S);
    SET_PERI_REG_BITS(PVT_DBIAS_CMD1_REG, PVT_DBIAS_CMD1_OFFSET_FLAG, flag, PVT_DBIAS_CMD1_OFFSET_FLAG_S);
    SET_PERI_REG_BITS(PVT_DBIAS_CMD1_REG, PVT_DBIAS_CMD1_OFFSET_VALUE, abs_value, PVT_DBIAS_CMD1_OFFSET_VALUE_S);
    SET_PERI_REG_BITS(PVT_DBIAS_CMD2_REG, PVT_DBIAS_CMD2_OFFSET_FLAG, flag, PVT_DBIAS_CMD2_OFFSET_FLAG_S);
    SET_PERI_REG_BITS(PVT_DBIAS_CMD2_REG, PVT_DBIAS_CMD2_OFFSET_VALUE, abs_value, PVT_DBIAS_CMD2_OFFSET_VALUE_S);
}

static uint32_t pvt_get_dcmvset(void)
{
    return GET_PERI_REG_BITS2(PMU_HP_ACTIVE_HP_REGULATOR0_REG, PMU_HP_DBIAS_VOL_V, PMU_HP_DBIAS_VOL_S);
}

static uint32_t pvt_get_lp_dbias(void)
{
    return GET_PERI_REG_BITS2(PMU_HP_ACTIVE_HP_REGULATOR0_REG, PMU_LP_DBIAS_VOL_V, PMU_LP_DBIAS_VOL_S);
}

static void pvt_auto_dbias_init(void)
{
    uint32_t blk_version = stub_target_get_blk_version();
    if (blk_version >= 2 && blk_version != 100) {
        REG_SET_BIT(HP_SYS_CLKRST_HP_RST_EN0_REG, HP_SYS_CLKRST_REG_RST_EN_PVT_PERI_GROUP4); // Must reset after pd_cpu
        REG_CLR_BIT(HP_SYS_CLKRST_HP_RST_EN0_REG, HP_SYS_CLKRST_REG_RST_EN_PVT_PERI_GROUP4);
        SET_PERI_REG_MASK(HP_SYS_CLKRST_REF_CLK_CTRL2_REG, HP_SYS_CLKRST_REG_REF_160M_CLK_EN);
        SET_PERI_REG_MASK(HP_SYS_CLKRST_SOC_CLK_CTRL1_REG, HP_SYS_CLKRST_REG_PVT_SYS_CLK_EN);
        /*config for dbias func*/
        CLEAR_PERI_REG_MASK(PVT_DBIAS_TIMER_REG, PVT_TIMER_EN);
        stub_lib_delay_us(1);
        SET_PERI_REG_BITS(PVT_DBIAS_CHANNEL_SEL0_REG, PVT_DBIAS_CHANNEL0_SEL, PVT_CHANNEL0_SEL, PVT_DBIAS_CHANNEL0_SEL_S);
        SET_PERI_REG_BITS(PVT_DBIAS_CHANNEL_SEL0_REG, PVT_DBIAS_CHANNEL1_SEL, PVT_CHANNEL1_SEL, PVT_DBIAS_CHANNEL1_SEL_S);
        SET_PERI_REG_BITS(PVT_DBIAS_CHANNEL0_SEL_REG, PVT_DBIAS_CHANNEL0_CFG, PVT_CHANNEL0_CFG, PVT_DBIAS_CHANNEL0_CFG_S);
        SET_PERI_REG_BITS(PVT_DBIAS_CHANNEL1_SEL_REG, PVT_DBIAS_CHANNEL1_CFG, PVT_CHANNEL1_CFG, PVT_DBIAS_CHANNEL1_CFG_S);
        SET_PERI_REG_BITS(PVT_DBIAS_CHANNEL2_SEL_REG, PVT_DBIAS_CHANNEL2_CFG, PVT_CHANNEL2_CFG, PVT_DBIAS_CHANNEL2_CFG_S);
        SET_PERI_REG_BITS(PVT_DBIAS_CMD0_REG, PVT_DBIAS_CMD0_PVT, PVT_CMD0, PVT_DBIAS_CMD0_PVT_S);
        SET_PERI_REG_BITS(PVT_DBIAS_CMD1_REG, PVT_DBIAS_CMD1_PVT, PVT_CMD1, PVT_DBIAS_CMD1_PVT_S);
        SET_PERI_REG_BITS(PVT_DBIAS_CMD2_REG, PVT_DBIAS_CMD2_PVT, PVT_CMD2, PVT_DBIAS_CMD2_PVT_S);
        SET_PERI_REG_BITS(PVT_DBIAS_TIMER_REG, PVT_TIMER_TARGET, PVT_TARGET, PVT_TIMER_TARGET_S);

        SET_PERI_REG_BITS(HP_SYS_CLKRST_PERI_CLK_CTRL24_REG, HP_SYS_CLKRST_REG_PVT_CLK_DIV_NUM, PVT_CLK_DIV,
                          HP_SYS_CLKRST_REG_PVT_CLK_DIV_NUM_S);
        SET_PERI_REG_BITS(HP_SYS_CLKRST_PERI_CLK_CTRL25_REG, HP_SYS_CLKRST_REG_PVT_PERI_GROUP_CLK_DIV_NUM, PVT_CLK_DIV,
                          HP_SYS_CLKRST_REG_PVT_PERI_GROUP_CLK_DIV_NUM_S);
        SET_PERI_REG_MASK(HP_SYS_CLKRST_PERI_CLK_CTRL24_REG, HP_SYS_CLKRST_REG_PVT_CLK_EN);
        SET_PERI_REG_MASK(HP_SYS_CLKRST_PERI_CLK_CTRL25_REG, HP_SYS_CLKRST_REG_PVT_PERI_GROUP1_CLK_EN);
        SET_PERI_REG_MASK(HP_SYS_CLKRST_PERI_CLK_CTRL25_REG, HP_SYS_CLKRST_REG_PVT_PERI_GROUP2_CLK_EN);
        SET_PERI_REG_MASK(HP_SYS_CLKRST_PERI_CLK_CTRL25_REG, HP_SYS_CLKRST_REG_PVT_PERI_GROUP3_CLK_EN);
        SET_PERI_REG_MASK(HP_SYS_CLKRST_PERI_CLK_CTRL25_REG, HP_SYS_CLKRST_REG_PVT_PERI_GROUP4_CLK_EN);

        /*config for pvt cell: unit0; site3; vt1*/
        SET_PERI_REG_BITS(PVT_COMB_PD_SITE3_UNIT0_VT1_CONF2_REG, PVT_MONITOR_EDG_MOD_VT1_PD_SITE3_UNIT0, PVT_EDG_MODE,
                          PVT_MONITOR_EDG_MOD_VT1_PD_SITE3_UNIT0_S);
        SET_PERI_REG_BITS(PVT_COMB_PD_SITE3_UNIT0_VT1_CONF1_REG, PVT_DELAY_LIMIT_VT1_PD_SITE3_UNIT0, PVT_DELAY_NUM_HIGH,
                          PVT_DELAY_LIMIT_VT1_PD_SITE3_UNIT0_S);
        SET_PERI_REG_BITS(PVT_COMB_PD_SITE3_UNIT1_VT1_CONF1_REG, PVT_DELAY_LIMIT_VT1_PD_SITE3_UNIT1, PVT_DELAY_NUM_LOW,
                          PVT_DELAY_LIMIT_VT1_PD_SITE3_UNIT1_S);

        /*config lp offset for pvt func*/
        uint8_t lp_hp_gap = get_lp_hp_gap();
        set_pvt_hp_lp_gap(lp_hp_gap);
    }
}

static void pvt_func_enable(bool enable)
{
    uint32_t blk_version = stub_target_get_blk_version();
    if (blk_version >= 2 && blk_version != 100) {
        if (enable) {
            SET_PERI_REG_MASK(HP_SYS_CLKRST_PERI_CLK_CTRL24_REG, HP_SYS_CLKRST_REG_PVT_CLK_EN);
            SET_PERI_REG_MASK(PMU_HP_ACTIVE_HP_REGULATOR0_REG, PMU_DIG_DBIAS_INIT);
            SET_PERI_REG_MASK(PVT_CLK_CFG_REG, PVT_MONITOR_CLK_PVT_EN);
            SET_PERI_REG_MASK(PVT_COMB_PD_SITE3_UNIT0_VT1_CONF1_REG, PVT_MONITOR_EN_VT1_PD_SITE3_UNIT0);
            stub_lib_delay_us(10);
            CLEAR_PERI_REG_MASK(PMU_HP_ACTIVE_HP_REGULATOR0_REG, PMU_DIG_REGULATOR0_DBIAS_SEL);
            CLEAR_PERI_REG_MASK(PMU_HP_ACTIVE_HP_REGULATOR0_REG, PMU_DIG_DBIAS_INIT);
            SET_PERI_REG_MASK(PVT_DBIAS_TIMER_REG, PVT_TIMER_EN);
            stub_lib_delay_us(50);
        } else {
            uint32_t pvt_dcmvset = pvt_get_dcmvset();
            uint32_t pvt_lpdbias = pvt_get_lp_dbias();
            SET_PERI_REG_BITS(PMU_HP_ACTIVE_BIAS_REG, PMU_HP_ACTIVE_DCM_VSET, pvt_dcmvset, PMU_HP_ACTIVE_DCM_VSET_S);
            SET_PERI_REG_BITS(PMU_HP_SLEEP_LP_REGULATOR0_REG, PMU_HP_SLEEP_LP_REGULATOR_DBIAS, pvt_lpdbias,
                              PMU_HP_SLEEP_LP_REGULATOR_DBIAS_S);
            SET_PERI_REG_MASK(PMU_HP_ACTIVE_HP_REGULATOR0_REG, PMU_DIG_REGULATOR0_DBIAS_SEL);
            CLEAR_PERI_REG_MASK(HP_SYS_CLKRST_PERI_CLK_CTRL24_REG, HP_SYS_CLKRST_REG_PVT_CLK_EN);
        }
    }
}

static void stub_target_switch_to_dcdc(void)
{
    unsigned chip_version = stub_target_get_chip_revision();
    uint32_t hp_dbias = stub_target_get_act_hp_dbias();
    uint32_t lp_dbias = stub_target_get_act_lp_dbias();
    /* Match IDF rtc_clk_init(): prefer PVT-sampled dcmvset when higher than default. */
    uint32_t pvt_hp_dcmvset = GET_PERI_REG_BITS2(PMU_HP_ACTIVE_HP_REGULATOR0_REG, PMU_HP_DBIAS_VOL_V, PMU_HP_DBIAS_VOL_S);
    uint32_t hp_dcmvset = HP_CALI_ACTIVE_DCM_VSET_DEFAULT;
    if (pvt_hp_dcmvset > hp_dcmvset) {
        hp_dcmvset = pvt_hp_dcmvset;
    }

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

/*
 * Minimal P4 regi2c write path (ESP-IDF esp_hal_regi2c/esp32p4/regi2c_impl.c).
 * ROM has ESP_ROM_WITHOUT_REGI2C; stub must drive LP_I2C_ANA_MST itself.
 */
#define REGI2C_PLL_CPU_MST_SEL  (BIT(11))
#define REGI2C_RTC_BUSY         (BIT(25))
#define REGI2C_RTC_WR_CNTL_S    24
#define REGI2C_RTC_DATA_S       16
#define REGI2C_RTC_ADDR_S       8
#define REGI2C_RTC_SLAVE_ID_S   0

static void stub_target_regi2c_master_enable(void)
{
    /* Match bootloader_esp32p4: keep ana i2c mst clock enabled + 160M sel. */
    SET_PERI_REG_MASK(LPPERI_CLK_EN_REG, LPPERI_CK_EN_LP_I2CMST);
    SET_PERI_REG_MASK(LP_I2C_ANA_MST_CLK160M_REG, LP_I2C_ANA_MST_CLK_I2C_MST_SEL_160M);
}

static void stub_target_regi2c_write(uint8_t block, uint8_t reg_add, uint8_t data)
{
    REG_SET_FIELD(LP_I2C_ANA_MST_ANA_CONF2_REG, LP_I2C_ANA_MST_ANA_CONF2, 0);
    REG_SET_FIELD(LP_I2C_ANA_MST_ANA_CONF1_REG, LP_I2C_ANA_MST_ANA_CONF1, 0);
    REG_SET_BIT(LP_I2C_ANA_MST_ANA_CONF2_REG, REGI2C_PLL_CPU_MST_SEL);

    while (REG_GET_BIT(LP_I2C_ANA_MST_I2C0_CTRL_REG, REGI2C_RTC_BUSY)) {
    }
    uint32_t temp = ((uint32_t)block << REGI2C_RTC_SLAVE_ID_S) | ((uint32_t)reg_add << REGI2C_RTC_ADDR_S) |
                    (1U << REGI2C_RTC_WR_CNTL_S) | ((uint32_t)data << REGI2C_RTC_DATA_S);
    REG_WRITE(LP_I2C_ANA_MST_I2C0_CTRL_REG, temp);
    while (REG_GET_BIT(LP_I2C_ANA_MST_I2C0_CTRL_REG, REGI2C_RTC_BUSY)) {
    }
}

static void stub_target_cpll_enable(void)
{
    SET_PERI_REG_MASK(PMU_IMM_HP_CK_POWER_REG, PMU_TIE_HIGH_XPD_CPLL | PMU_TIE_HIGH_XPD_CPLL_I2C);
    SET_PERI_REG_MASK(PMU_IMM_HP_CK_POWER_REG, PMU_TIE_HIGH_GLOBAL_CPLL_ICG);
}

/*
 * Ported from IDF clk_ll_cpll_set_config() + rtc_clk_cpll_configure().
 * Must run while CPU root is on XTAL (not CPLL).
 */
static void stub_target_cpll_configure(uint32_t xtal_freq_mhz, uint32_t cpll_freq_mhz)
{
    uint8_t div_ref;
    uint8_t div7_0;
    const uint8_t dchgp = 5;
    const uint8_t dcur = 3;
    const uint8_t oc_enb_fcal = 0;
    unsigned chip_version = stub_target_get_chip_revision();

    (void)xtal_freq_mhz; /* Only 40 MHz XTAL is supported on P4. */

    /* Match IDF !ESP_CHIP_REV_ABOVE(rev, 1): ECO1+ uses swapped div7_0 encoding. */
    if (chip_version < 1U) {
        div7_0 = (cpll_freq_mhz == CPLL_FREQ_MHZ) ? 6 : 5;
        div_ref = 0;
    } else {
        div7_0 = (cpll_freq_mhz == CPLL_FREQ_MHZ) ? 10 : 9;
        div_ref = 0;
    }

    uint8_t i2c_cpll_lref = (uint8_t)((oc_enb_fcal << I2C_CPLL_OC_ENB_FCAL_LSB) |
                                      (dchgp << I2C_CPLL_OC_DCHGP_LSB) | div_ref);
    uint8_t i2c_cpll_dcur =
        (uint8_t)((1U << I2C_CPLL_OC_DLREF_SEL_LSB) | (3U << I2C_CPLL_OC_DHREF_SEL_LSB) | dcur);

    stub_target_regi2c_master_enable();

    /* CPLL calibration start */
    CLEAR_PERI_REG_MASK(HP_SYS_CLKRST_ANA_PLL_CTRL0_REG, HP_SYS_CLKRST_REG_CPU_PLL_CAL_STOP);
    stub_target_regi2c_write(I2C_CPLL, I2C_CPLL_OC_REF_DIV, i2c_cpll_lref);
    stub_target_regi2c_write(I2C_CPLL, I2C_CPLL_OC_DIV_7_0, div7_0);
    stub_target_regi2c_write(I2C_CPLL, I2C_CPLL_OC_DCUR, i2c_cpll_dcur);
    while ((REG_READ(HP_SYS_CLKRST_ANA_PLL_CTRL0_REG) & HP_SYS_CLKRST_REG_CPU_PLL_CAL_END) == 0) {
    }
    stub_lib_delay_us(10);
    SET_PERI_REG_MASK(HP_SYS_CLKRST_ANA_PLL_CTRL0_REG, HP_SYS_CLKRST_REG_CPU_PLL_CAL_STOP);
}

/*
 * Match IDF rtc_clk_cpu_freq_to_xtal(xtal, 1, false):
 * switch root to XTAL first, then set CPU/MEM/SYS/APB dividers to 1.
 */
static void stub_target_cpu_freq_to_xtal(void)
{
    uint32_t ctrl0;

    REG_SET_FIELD(LP_CLKRST_HP_CLK_CTRL_REG, LP_CLKRST_HP_ROOT_CLK_SRC_SEL, 0);

    ctrl0 = READ_PERI_REG(HP_SYS_CLKRST_ROOT_CLK_CTRL0_REG);
    ctrl0 &= ~(HP_SYS_CLKRST_REG_CPU_CLK_DIV_NUM_M | HP_SYS_CLKRST_REG_CPU_CLK_DIV_NUMERATOR_M |
               HP_SYS_CLKRST_REG_CPU_CLK_DIV_DENOMINATOR_M);
    WRITE_PERI_REG(HP_SYS_CLKRST_ROOT_CLK_CTRL0_REG, ctrl0);
    REG_SET_FIELD(HP_SYS_CLKRST_ROOT_CLK_CTRL1_REG, HP_SYS_CLKRST_REG_MEM_CLK_DIV_NUM, 0);
    REG_SET_FIELD(HP_SYS_CLKRST_ROOT_CLK_CTRL1_REG, HP_SYS_CLKRST_REG_SYS_CLK_DIV_NUM, 0);
    REG_SET_FIELD(HP_SYS_CLKRST_ROOT_CLK_CTRL2_REG, HP_SYS_CLKRST_REG_APB_CLK_DIV_NUM, 0);
    stub_target_bus_update();

    esp_rom_set_cpu_ticks_per_us(XTAL_FREQ_MHZ);
}

/*
 * Match IDF rtc_clk_cpu_freq_to_cpll_mhz(400) for rev >= v3:
 * CPLL 400 /1 -> CPU 400, MEM /2 -> 200, SYS /1 -> 200, APB /2 -> 100.
 * Divider register fields store (divider - 1). Upscale order: APB -> SYS -> MEM -> CPU -> mux.
 */
static void stub_target_apply_cpu_cpll_div1(void)
{
    uint32_t ctrl0;

    REG_SET_FIELD(HP_SYS_CLKRST_ROOT_CLK_CTRL2_REG, HP_SYS_CLKRST_REG_APB_CLK_DIV_NUM, 1);
    stub_target_bus_update();
    REG_SET_FIELD(HP_SYS_CLKRST_ROOT_CLK_CTRL1_REG, HP_SYS_CLKRST_REG_SYS_CLK_DIV_NUM, 0);
    stub_target_bus_update();
    REG_SET_FIELD(HP_SYS_CLKRST_ROOT_CLK_CTRL1_REG, HP_SYS_CLKRST_REG_MEM_CLK_DIV_NUM, 1);
    stub_target_bus_update();

    /* 400 / 1 = 400 */
    ctrl0 = READ_PERI_REG(HP_SYS_CLKRST_ROOT_CLK_CTRL0_REG);
    ctrl0 &= ~(HP_SYS_CLKRST_REG_CPU_CLK_DIV_NUM_M | HP_SYS_CLKRST_REG_CPU_CLK_DIV_NUMERATOR_M |
               HP_SYS_CLKRST_REG_CPU_CLK_DIV_DENOMINATOR_M);
    WRITE_PERI_REG(HP_SYS_CLKRST_ROOT_CLK_CTRL0_REG, ctrl0);
    stub_target_bus_update();

    REG_SET_FIELD(LP_CLKRST_HP_CLK_CTRL_REG, LP_CLKRST_HP_ROOT_CLK_SRC_SEL, 1);
}


void stub_target_clock_init(void)
{
    /* Bootloader-equivalent: DCDC first (rtc_clk_init). */
    stub_target_switch_to_dcdc();

    /* App-equivalent: pmu_init() enables PVT before final CPU boost. */
    pvt_auto_dbias_init();
    pvt_func_enable(true);
    stub_lib_delay_us(PVT_SETTLE_US);

    /*
     * App-equivalent: rtc_clk_cpu_freq_set_config(CPLL 400 /1).
     * Recalibrating CPLL requires switching CPU root to XTAL first.
     */
    stub_target_cpll_enable();
    stub_target_cpu_freq_to_xtal();
    stub_target_cpll_configure(XTAL_FREQ_MHZ, CPLL_FREQ_MHZ);
    stub_target_apply_cpu_cpll_div1();

    s_cpu_freq = CPU_FREQ_MHZ * MHZ;
    esp_rom_set_cpu_ticks_per_us(CPU_FREQ_MHZ);

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
