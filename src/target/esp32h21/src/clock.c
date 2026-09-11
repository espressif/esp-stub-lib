/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <soc_utils.h>

#include <esp-stub-lib/rom_wrappers.h>

#include <target/clock.h>

#include <soc/i2c_ana_mst_reg.h>
#include <soc/lp_wdt_reg.h>
#include <soc/pcr_reg.h>
#include <soc/pmu_reg.h>
#include <soc/regi2c_dcdc.h>
#include <soc/soc.h>

/* CPU clock sources: 0 = XTAL (32 MHz), 1 = SPLL, 2 = FOSC, 3 = XTAL_X2. */
#define SOC_CPU_CLK_SRC_PLL 1U

/* 96 MHz is the highest frequency ESP32-H21 supports (SPLL, CPU divider of 1). */
#define CPU_FREQ_MHZ        96

/*
 * AHB_CLK is derived from CPU_CLK and must not exceed 32 MHz, so the CPU
 * divider of 1 pairs with an AHB divider of 3.
 */
#define CPU_DIV             1U
#define AHB_DIV             3U

/*
 * The core rail is supplied by a switching DC-DC converter, which ESP32-H21
 * shares only with ESP32-H4. The ROM leaves it in its reset state: discontinuous
 * conduction mode, which is the light-load mode suited to the 32 MHz the chip
 * boots at. It has to be brought up before the CPU clock is raised.
 */

/* Output voltage setting, shared by both conduction modes. */
#define DCDC_DREG_VALUE     12U

/* Peak current limits, which have to allow for the higher load at 96 MHz. */
#define DCDC_CCM_PCUR_LIMIT 4U
#define DCDC_VCM_PCUR_LIMIT 2U

/* Drive strength of the high power regulator once the converter supplies it. */
#define HP_REGULATOR_DRV_B  8U

/* Time for the rail to settle before the CPU clock is raised. */
#define DCDC_SETTLE_US      1000U

extern uint32_t esp_rom_get_cpu_freq(void);
extern void esp_rom_set_cpu_ticks_per_us(uint32_t ticks_per_us);

static uint32_t s_cpu_freq = 0;

/*
 * Selects the converter on the analog I2C bus and returns which of the two I2C
 * masters now owns it. The ROM exports no regi2c API, so the transaction below
 * is driven directly.
 */
static uint32_t regi2c_select_dcdc(void)
{
    uint32_t sel = REG_GET_BIT(I2C_ANA_MST_ANA_CONF2_REG, REGI2C_CONF2_PMU_SEL);

    REG_WRITE(I2C_ANA_MST_ANA_CONF1_REG, ~REGI2C_CONF1_PMU_SEL & I2C_ANA_MST_ANA_CONF1_M);

    return sel ? 0U : 1U;
}

/* Read-modify-write of a bit field in one converter register. */
static void regi2c_write_mask(uint32_t reg, uint32_t msb, uint32_t lsb, uint32_t data)
{
    uint32_t ctrl = I2C_ANA_MST_I2C_CTRL_REG(regi2c_select_dcdc());
    uint32_t mask = (1U << (msb - lsb + 1U)) - 1U;
    uint32_t temp;

    while (REG_GET_BIT(ctrl, REGI2C_RTC_BUSY)) {
    }

    /* Issue a read of the register so the untouched bits can be preserved. */
    REG_WRITE(ctrl,
              ((I2C_DCDC & REGI2C_RTC_SLAVE_ID_V) << REGI2C_RTC_SLAVE_ID_S) |
                  ((reg & REGI2C_RTC_ADDR_V) << REGI2C_RTC_ADDR_S));
    while (REG_GET_BIT(ctrl, REGI2C_RTC_BUSY)) {
    }

    temp = (REG_READ(ctrl) >> REGI2C_RTC_DATA_S) & REGI2C_RTC_DATA_V;
    temp &= ~(mask << lsb);
    temp |= (data & mask) << lsb;

    REG_WRITE(ctrl,
              ((I2C_DCDC & REGI2C_RTC_SLAVE_ID_V) << REGI2C_RTC_SLAVE_ID_S) |
                  ((reg & REGI2C_RTC_ADDR_V) << REGI2C_RTC_ADDR_S) |
                  ((1U & REGI2C_RTC_WR_CNTL_V) << REGI2C_RTC_WR_CNTL_S) |
                  ((temp & REGI2C_RTC_DATA_V) << REGI2C_RTC_DATA_S));
    while (REG_GET_BIT(ctrl, REGI2C_RTC_BUSY)) {
    }
}

static void dcdc_init(void)
{
    /* The analog I2C master is clock gated out of reset. */
    REG_SET_BIT(MODEM_LPCON_CLK_CONF_REG, MODEM_LPCON_CLK_I2C_MST_EN);

    /* Put the converter into continuous conduction mode. */
    REG_SET_BIT(PMU_DCM_CTRL_REG, PMU_DCDC_CCM_SW_EN);
    REG_CLR_BIT(PMU_HP_ACTIVE_BIAS_REG, PMU_HP_ACTIVE_DCDC_CCM_ENB);

    regi2c_write_mask(I2C_DCDC_CCM_DREG0, I2C_DCDC_CCM_DREG0_MSB, I2C_DCDC_CCM_DREG0_LSB, DCDC_DREG_VALUE);
    regi2c_write_mask(I2C_DCDC_CCM_PCUR_LIMIT0,
                      I2C_DCDC_CCM_PCUR_LIMIT0_MSB,
                      I2C_DCDC_CCM_PCUR_LIMIT0_LSB,
                      DCDC_CCM_PCUR_LIMIT);
    regi2c_write_mask(I2C_DCDC_VCM_DREG0, I2C_DCDC_VCM_DREG0_MSB, I2C_DCDC_VCM_DREG0_LSB, DCDC_DREG_VALUE);
    regi2c_write_mask(I2C_DCDC_VCM_PCUR_LIMIT0,
                      I2C_DCDC_VCM_PCUR_LIMIT0_MSB,
                      I2C_DCDC_VCM_PCUR_LIMIT0_LSB,
                      DCDC_VCM_PCUR_LIMIT);
    regi2c_write_mask(I2C_DCDC_XPD_TRX, I2C_DCDC_XPD_TRX_MSB, I2C_DCDC_XPD_TRX_LSB, 0U);

    /* The RF PLL is of no use to the stub and only loads the rail. */
    REG_CLR_BIT(PMU_DATE_REG, PMU_RF_PLL_EN);

    REG_SET_FIELD(PMU_HP_ACTIVE_HP_REGULATOR1_REG, PMU_HP_ACTIVE_HP_REGULATOR_DRV_B, HP_REGULATOR_DRV_B);

    stub_lib_delay_us(DCDC_SETTLE_US);
}

static void update_bus_clocks(void)
{
    REG_SET_FIELD(PCR_BUS_CLK_UPDATE_REG, PCR_BUS_CLOCK_UPDATE, 1);
    while (REG_GET_BIT(PCR_BUS_CLK_UPDATE_REG, PCR_BUS_CLOCK_UPDATE)) {
    }
}

void stub_target_clock_init(void)
{
    s_cpu_freq = CPU_FREQ_MHZ * MHZ;
    esp_rom_set_cpu_ticks_per_us(CPU_FREQ_MHZ);

    /* The rail has to carry 96 MHz before the CPU clock is raised. */
    dcdc_init();

    /* Program the dividers before switching the root mux. */
    REG_SET_FIELD(PCR_CPU_FREQ_CONF_REG, PCR_CPU_DIV_NUM, CPU_DIV - 1U);
    REG_SET_FIELD(PCR_AHB_FREQ_CONF_REG, PCR_AHB_DIV_NUM, AHB_DIV - 1U);
    REG_SET_FIELD(PCR_SYSCLK_CONF_REG, PCR_SOC_CLK_SEL, SOC_CPU_CLK_SRC_PLL);
    update_bus_clocks();
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
