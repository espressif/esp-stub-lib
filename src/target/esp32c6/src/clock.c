/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <soc_utils.h>

#include <esp-stub-lib/bit_utils.h>

#include <target/clock.h>

#include <soc/efuse_reg.h>
#include <soc/i2c_ana_mst_reg.h>
#include <soc/lp_wdt_reg.h>
#include <soc/modem_lpcon_reg.h>
#include <soc/modem_syscon_reg.h>
#include <soc/pcr_reg.h>
#include <soc/pmu_reg.h>
#include <soc/regi2c_dig_reg.h>
#include <soc/soc.h>

#define CPU_FREQ_MHZ                 160

/* CPU clock sources: 0 = XTAL, 1 = PLL (480 MHz), 2 = RC_FAST. */
#define SOC_CPU_CLK_SRC_PLL          1U

/* 480 MHz / 3 (fixed HP_ROOT divider) / 1 = 160 MHz. */
#define CPU_HS_DIV                   1U

/* MSPI_FAST_CLK = 480 MHz / 6 = 80 MHz. The reset divider of 4 gives 120 MHz,
 * which is unusable before MSPI timing tuning. */
#define MSPI_FAST_HS_DIV             6U

/* ICG bitmap bit of the PMU HP_ACTIVE state. */
#define PMU_HP_ICG_MODEM_CODE_ACTIVE 2U

/* ESP-IDF's dbias values for chips without eFuse calibration. */
#define HP_CALI_DBIAS_DEFAULT        25U
#define LP_CALI_DBIAS_DEFAULT        26U

/* First eFuse block version (major * 100 + minor) carrying dbias calibration. */
#define DBIAS_CALI_BLK_VERSION       3U
/* Margin ESP-IDF adds to the eFuse dbias to meet CPU frequency switching. */
#define DBIAS_CALI_MARGIN            2U
#define DBIAS_MAX                    31U

extern uint32_t esp_rom_get_cpu_freq(void);
extern void esp_rom_set_cpu_ticks_per_us(uint32_t ticks_per_us);

static uint32_t s_cpu_freq = 0;

/*
 * The MODEM_APB, I2C_MST and LP_APB clock domains are gated in the PMU_ACTIVE
 * state out of reset, and the analog I2C master sits behind them.
 */
static void modem_clock_domain_active_state_icg_map_preinit(void)
{
    REG_SET_FIELD(PMU_HP_ACTIVE_ICG_MODEM_REG, PMU_HP_ACTIVE_DIG_ICG_MODEM_CODE, PMU_HP_ICG_MODEM_CODE_ACTIVE);

    REG_SET_FIELD(MODEM_SYSCON_CLK_CONF_POWER_ST_REG,
                  MODEM_SYSCON_CLK_MODEM_APB_ST_MAP,
                  BIT(PMU_HP_ICG_MODEM_CODE_ACTIVE));
    REG_SET_FIELD(MODEM_LPCON_CLK_CONF_POWER_ST_REG, MODEM_LPCON_CLK_I2C_MST_ST_MAP, BIT(PMU_HP_ICG_MODEM_CODE_ACTIVE));
    REG_SET_FIELD(MODEM_LPCON_CLK_CONF_POWER_ST_REG, MODEM_LPCON_CLK_LP_APB_ST_MAP, BIT(PMU_HP_ICG_MODEM_CODE_ACTIVE));

    REG_SET_BIT(PMU_IMM_MODEM_ICG_REG, PMU_UPDATE_DIG_ICG_MODEM_EN);
    REG_SET_BIT(PMU_IMM_SLEEP_SYSCLK_REG, PMU_UPDATE_DIG_ICG_SWITCH);
}

/* Selects the digital regulator on the analog I2C bus and returns the master to use. */
static uint32_t regi2c_select_dig_reg(void)
{
    uint32_t sel = REG_GET_BIT(I2C_ANA_MST_ANA_CONF2_REG, REGI2C_CONF2_DIG_REG_SEL);

    REG_WRITE(I2C_ANA_MST_ANA_CONF1_REG, ~REGI2C_CONF1_DIG_REG_SEL & I2C_ANA_MST_ANA_CONF1_M);

    return sel ? 0U : 1U;
}

/* Read-modify-write of a bit field in one digital regulator register. */
static void regi2c_write_mask(uint32_t reg, uint32_t msb, uint32_t lsb, uint32_t data)
{
    uint32_t ctrl = I2C_ANA_MST_I2C_CTRL_REG(regi2c_select_dig_reg());
    uint32_t mask = BIT(msb - lsb + 1U) - 1U;
    uint32_t temp;

    while (REG_GET_BIT(ctrl, REGI2C_RTC_BUSY)) {
    }

    /* Issue a read of the register so the untouched bits can be preserved. */
    REG_WRITE(ctrl,
              ((I2C_DIG_REG & REGI2C_RTC_SLAVE_ID_V) << REGI2C_RTC_SLAVE_ID_S) |
                  ((reg & REGI2C_RTC_ADDR_V) << REGI2C_RTC_ADDR_S));
    while (REG_GET_BIT(ctrl, REGI2C_RTC_BUSY)) {
    }

    temp = REG_GET_FIELD(ctrl, REGI2C_RTC_DATA);
    temp &= ~(mask << lsb);
    temp |= (data & mask) << lsb;

    REG_WRITE(ctrl,
              ((I2C_DIG_REG & REGI2C_RTC_SLAVE_ID_V) << REGI2C_RTC_SLAVE_ID_S) |
                  ((reg & REGI2C_RTC_ADDR_V) << REGI2C_RTC_ADDR_S) | REGI2C_RTC_WR_CNTL |
                  ((temp & REGI2C_RTC_DATA_V) << REGI2C_RTC_DATA_S));
    while (REG_GET_BIT(ctrl, REGI2C_RTC_BUSY)) {
    }
}

static uint32_t get_act_dbias(uint32_t efuse_dbias, uint32_t default_dbias)
{
    uint32_t major = REG_GET_FIELD(EFUSE_RD_MAC_SPI_SYS_3_REG, EFUSE_BLK_VERSION_MAJOR);
    uint32_t minor = REG_GET_FIELD(EFUSE_RD_MAC_SPI_SYS_3_REG, EFUSE_BLK_VERSION_MINOR);

    if (major * 100U + minor < DBIAS_CALI_BLK_VERSION || efuse_dbias == 0) {
        return default_dbias;
    }
    return MIN(efuse_dbias + DBIAS_CALI_MARGIN, DBIAS_MAX);
}

/*
 * Out of reset the regulators ignore the PMU and run at their analog default,
 * so hand them over to the PMU and set the calibrated core voltage.
 */
static void dbias_init(void)
{
    /* The analog I2C master is clock gated out of reset, and its default clock
     * source leaves transactions intermittently stuck busy. */
    REG_SET_BIT(MODEM_LPCON_CLK_CONF_REG, MODEM_LPCON_CLK_I2C_MST_EN);
    REG_SET_BIT(MODEM_LPCON_I2C_MST_CLK_CONF_REG, MODEM_LPCON_CLK_I2C_MST_SEL_160M);

    regi2c_write_mask(I2C_DIG_REG_ENIF_RTC_DREG, I2C_DIG_REG_ENIF_RTC_DREG_MSB, I2C_DIG_REG_ENIF_RTC_DREG_LSB, 1U);
    regi2c_write_mask(I2C_DIG_REG_ENIF_DIG_DREG, I2C_DIG_REG_ENIF_DIG_DREG_MSB, I2C_DIG_REG_ENIF_DIG_DREG_LSB, 1U);

    uint32_t hp_cali_dbias =
        get_act_dbias(REG_GET_FIELD(EFUSE_RD_MAC_SPI_SYS_2_REG, EFUSE_ACTIVE_HP_DBIAS), HP_CALI_DBIAS_DEFAULT);
    uint32_t lp_cali_dbias =
        get_act_dbias(REG_GET_FIELD(EFUSE_RD_MAC_SPI_SYS_2_REG, EFUSE_ACTIVE_LP_DBIAS), LP_CALI_DBIAS_DEFAULT);

    REG_SET_BIT(PMU_HP_ACTIVE_HP_REGULATOR0_REG, PMU_DIG_REGULATOR0_DBIAS_SEL);

    REG_SET_FIELD(PMU_HP_ACTIVE_HP_REGULATOR0_REG, PMU_HP_ACTIVE_HP_REGULATOR_DBIAS, hp_cali_dbias);
    REG_SET_FIELD(PMU_HP_MODEM_HP_REGULATOR0_REG, PMU_HP_MODEM_HP_REGULATOR_DBIAS, hp_cali_dbias);
    REG_SET_FIELD(PMU_HP_SLEEP_LP_REGULATOR0_REG, PMU_HP_SLEEP_LP_REGULATOR_DBIAS, lp_cali_dbias);
}

/*
 * The C6 ROM's SPI_init() resets the MSPI fast clock HS divider to a wrong
 * default of /4, which is 120 MHz with the CPU on PLL and unusable without
 * MSPI timing tuning. The HS divider has no effect while the CPU runs from XTAL.
 */
void stub_target_clock_restore_mspi(void)
{
    REG_SET_FIELD(PCR_MSPI_CLK_CONF_REG, PCR_MSPI_FAST_HS_DIV_NUM, MSPI_FAST_HS_DIV - 1U);
}

void stub_target_clock_init(void)
{
    modem_clock_domain_active_state_icg_map_preinit();

    /* The core rail has to carry 160 MHz before the CPU clock is raised. */
    dbias_init();

    s_cpu_freq = CPU_FREQ_MHZ * MHZ;
    esp_rom_set_cpu_ticks_per_us(CPU_FREQ_MHZ);

    /* Program the dividers before switching the root mux. */
    stub_target_clock_restore_mspi();
    REG_SET_FIELD(PCR_CPU_FREQ_CONF_REG, PCR_CPU_HS_DIV_NUM, CPU_HS_DIV - 1U);
    REG_CLR_BIT(PCR_CPU_FREQ_CONF_REG, PCR_CPU_HS_120M_FORCE);
    REG_SET_FIELD(PCR_SYSCLK_CONF_REG, PCR_SOC_CLK_SEL, SOC_CPU_CLK_SRC_PLL);
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
