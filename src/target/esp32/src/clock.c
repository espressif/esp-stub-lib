/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <esp-stub-lib/rom_wrappers.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/clock.h>

#include <soc/dport_reg.h>
#include <soc/reg_base.h>
#include <soc/regi2c_bbpll.h>
#include <soc/regi2c_defs.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/soc.h>
#include <soc/syscon_reg.h>

/*
 * Some ESP32 parts are eFuse-rated for a maximum CPU frequency of 160 MHz.
 * Keep the stub at 160 MHz so the same binary is safe for every ESP32.
 */
#define CPU_FREQ_MHZ                     160

#define CLK_LL_BBPLL_IR_CAL_DELAY_VAL    0x18U
#define CLK_LL_BBPLL_IR_CAL_EXT_CAP_VAL  0x20U
#define CLK_LL_BBPLL_OC_ENB_FCAL_VAL     0x9AU
#define CLK_LL_BBPLL_OC_ENB_VCON_VAL     0x00U
#define CLK_LL_BBPLL_BBADC_CAL_7_0_VAL   0x00U

#define CLK_LL_BBPLL_ENDIV5_VAL_320M     0x43U
#define CLK_LL_BBPLL_BBADC_DSMP_VAL_320M 0x84U

extern void esp_rom_set_cpu_ticks_per_us(uint32_t ticks_per_us);
extern uint32_t esp_rom_get_cpu_ticks_per_us(void);
extern uint32_t esp_rom_get_detected_xtal_freq(void);
extern void rom_i2c_writeReg(uint32_t block, uint32_t host_id, uint32_t reg_add, uint32_t data);

static uint32_t s_cpu_freq = 0;

static void stub_esp32_bbpll_write(uint32_t reg, uint32_t value)
{
    rom_i2c_writeReg(I2C_BBPLL, I2C_BBPLL_HOSTID, reg, value);
}

static void stub_esp32_bbpll_configure_320m(uint32_t xtal_freq_mhz)
{
    /* Default to the most common 40 MHz XTAL configuration. */
    uint32_t div_ref = 0U;
    uint32_t div7_0 = 32U;
    uint32_t div10_8 = 0U;
    uint32_t lref = 0U;
    uint32_t dcur = 6U;
    uint32_t bw = 3U;

    if (xtal_freq_mhz == 26U) {
        div_ref = 12U;
        div7_0 = 224U;
        div10_8 = 4U;
        lref = 1U;
        dcur = 0U;
        bw = 1U;
    }

    /* Power up the internal analog-I2C bus and BBPLL. */
    REG_CLR_BIT(RTC_CNTL_OPTIONS0_REG,
                RTC_CNTL_BIAS_I2C_FORCE_PD | RTC_CNTL_BB_I2C_FORCE_PD | RTC_CNTL_BBPLL_FORCE_PD |
                    RTC_CNTL_BBPLL_I2C_FORCE_PD);
    REG_CLR_BIT(ANA_CONFIG_REG, I2C_BBPLL_M);

    /* Reset and configure BBPLL exactly as ESP-IDF does for a 320 MHz PLL. */
    stub_esp32_bbpll_write(I2C_BBPLL_IR_CAL_DELAY, CLK_LL_BBPLL_IR_CAL_DELAY_VAL);
    stub_esp32_bbpll_write(I2C_BBPLL_IR_CAL_EXT_CAP, CLK_LL_BBPLL_IR_CAL_EXT_CAP_VAL);
    stub_esp32_bbpll_write(I2C_BBPLL_OC_ENB_FCAL, CLK_LL_BBPLL_OC_ENB_FCAL_VAL);
    stub_esp32_bbpll_write(I2C_BBPLL_OC_ENB_VCON, CLK_LL_BBPLL_OC_ENB_VCON_VAL);
    stub_esp32_bbpll_write(I2C_BBPLL_BBADC_CAL_7_0, CLK_LL_BBPLL_BBADC_CAL_7_0_VAL);
    stub_esp32_bbpll_write(I2C_BBPLL_ENDIV5, CLK_LL_BBPLL_ENDIV5_VAL_320M);
    stub_esp32_bbpll_write(I2C_BBPLL_BBADC_DSMP, CLK_LL_BBPLL_BBADC_DSMP_VAL_320M);
    stub_esp32_bbpll_write(I2C_BBPLL_OC_LREF, (lref << 7) | (div10_8 << 4) | div_ref);
    stub_esp32_bbpll_write(I2C_BBPLL_OC_DIV_7_0, div7_0);
    stub_esp32_bbpll_write(I2C_BBPLL_OC_DCUR, (bw << 6) | dcur);

    stub_lib_delay_us(80U);
}

void stub_target_clock_init(void)
{
    uint32_t xtal_freq_hz = esp_rom_get_detected_xtal_freq();
    uint32_t xtal_freq_mhz = 40U;
    if (xtal_freq_hz != 0U && xtal_freq_hz < 30500000U) {
        xtal_freq_mhz = 26U;
    }

    /* Reconfigure the PLL while the CPU is still running from XTAL. */
    REG_SET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_SOC_CLK_SEL, 0U);
    esp_rom_set_cpu_ticks_per_us(xtal_freq_mhz);
    REG_SET_FIELD(RTC_CNTL_REG, RTC_CNTL_DIG_DBIAS_WAK, RTC_CNTL_DBIAS_1V25);
    stub_esp32_bbpll_configure_320m(xtal_freq_mhz);

    /* 320 MHz PLL / 2 gives the requested 160 MHz CPU clock. */
    DPORT_REG_WRITE(DPORT_CPU_PER_CONF_REG, 1U);

    /* REF_TICK stays at 1 MHz and APB is fixed at 80 MHz on the PLL path. */
    WRITE_PERI_REG(SYSCON_PLL_TICK_CONF_REG, 80U - 1U);
    uint32_t apb_store = (80U * MHZ) >> 12;
    WRITE_PERI_REG(RTC_CNTL_STORE5_REG, apb_store | (apb_store << 16));

    s_cpu_freq = CPU_FREQ_MHZ * MHZ;
    esp_rom_set_cpu_ticks_per_us(CPU_FREQ_MHZ);
    REG_SET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_SOC_CLK_SEL, 1U);
    stub_lib_delay_us(30U);
}

uint32_t stub_target_get_cpu_freq(void)
{
    if (s_cpu_freq == 0) {
        return esp_rom_get_cpu_ticks_per_us() * MHZ;
    }
    return s_cpu_freq;
}

#define RTC_CNTL_WDT_KEY 0x50D83AA1

void stub_target_clock_disable_watchdogs(void)
{
    // Disable RWDT (RTC Watchdog)
    REG_SET_BIT(RTC_CNTL_INT_CLR_REG, RTC_CNTL_WDT_INT_CLR);
    WRITE_PERI_REG(RTC_CNTL_WDTWPROTECT_REG, RTC_CNTL_WDT_KEY);
    WRITE_PERI_REG(RTC_CNTL_WDTCONFIG0_REG, 0x0);
    WRITE_PERI_REG(RTC_CNTL_WDTWPROTECT_REG, 0x0);
}
