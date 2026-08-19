/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>

#include <esp-stub-lib/rom_wrappers.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/clock.h>

#include <soc/efuse_reg.h>
#include <soc/regi2c_bbpll.h>
#include <soc/regi2c_defs.h>
#include <soc/regi2c_dig_reg.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/soc.h>
#include <soc/system_reg.h>

#define CPU_FREQ_MHZ              240

/* ESP-IDF's default bootloader DBIAS values, used when eFuse PVT data is unavailable. */
#define DIG_DBIAS_240M_DEFAULT    28U
#define RTC_DBIAS_240M_DEFAULT    28U

/* ESP-IDF's nominal S3 LDO calibration values, scaled by 10000. */
#define K_RTC_MID_MUL10000        198
#define K_DIG_MID_MUL10000        211
#define V_RTC_MID_MUL10000        10181
#define V_DIG_MID_MUL10000        10841
#define V_DIG_1V3_MUL10000        13000

#define DBIAS_MIN                 15U
#define DBIAS_MAX                 31U
#define DBIAS_RTC_TOLERANCE       250
#define DBIAS_CALIBRATION_DIVISOR 500
#define DBIAS_SETTLE_US           40

#define DEFAULT_LDO_SLAVE         0x7U

extern uint32_t esp_rom_get_cpu_ticks_per_us(void);
extern void esp_rom_set_cpu_ticks_per_us(uint32_t ticks_per_us);
extern uint32_t esp_rom_get_xtal_freq(void);
extern uint32_t esp_rom_get_apb_frequency(void);
extern void esp_rom_regi2c_write(uint8_t block, uint8_t host_id, uint8_t reg_add, uint8_t data);
extern void
esp_rom_regi2c_write_mask(uint8_t block, uint8_t host_id, uint8_t reg_add, uint8_t msb, uint8_t lsb, uint8_t data);

static uint32_t s_cpu_freq = 0;

static uint32_t efuse_field(uint32_t reg, uint32_t mask, unsigned shift)
{
    return (REG_READ(reg) >> shift) & mask;
}

static int32_t sign_magnitude(uint32_t value, uint32_t sign_bit, uint32_t magnitude_mask)
{
    return (value & sign_bit) ? -(int32_t)(value & magnitude_mask) : (int32_t)value;
}

static bool has_pvt_dbias_calibration(void)
{
    uint32_t major = efuse_field(EFUSE_RD_SYS_PART1_DATA4_REG, EFUSE_BLK_VERSION_MAJOR_V, EFUSE_BLK_VERSION_MAJOR_S);
    uint32_t minor = efuse_field(EFUSE_RD_MAC_SPI_SYS_3_REG, EFUSE_BLK_VERSION_MINOR_V, EFUSE_BLK_VERSION_MINOR_S);

    /* Keep the same block-version test used by ESP-IDF's rtc_set_stored_dbias(). */
    return (major <= 1U && minor == 1U) || major > 1U || (major == 1U && minor >= 2U);
}

static int32_t get_k_rtc_ldo(void)
{
    uint32_t raw = efuse_field(EFUSE_RD_MAC_SPI_SYS_4_REG, EFUSE_K_RTC_LDO_V, EFUSE_K_RTC_LDO_S);
    return sign_magnitude(raw, BIT(6), 0x3FU);
}

static int32_t get_k_dig_ldo(void)
{
    uint32_t raw = efuse_field(EFUSE_RD_MAC_SPI_SYS_4_REG, EFUSE_K_DIG_LDO_V, EFUSE_K_DIG_LDO_S);
    return sign_magnitude(raw, BIT(6), 0x3FU);
}

static int32_t get_v_rtc_dbias20(void)
{
    uint32_t low = efuse_field(EFUSE_RD_MAC_SPI_SYS_4_REG, EFUSE_V_RTC_DBIAS20_V, EFUSE_V_RTC_DBIAS20_S);
    uint32_t high = efuse_field(EFUSE_RD_MAC_SPI_SYS_5_REG, EFUSE_V_RTC_DBIAS20_1_V, EFUSE_V_RTC_DBIAS20_1_S);
    return sign_magnitude((high << 5) | low, BIT(7), 0x7FU);
}

static int32_t get_v_dig_dbias20(void)
{
    uint32_t raw = efuse_field(EFUSE_RD_MAC_SPI_SYS_5_REG, EFUSE_V_DIG_DBIAS20_V, EFUSE_V_DIG_DBIAS20_S);
    return sign_magnitude(raw, BIT(7), 0x7FU);
}

static uint32_t get_dig_1v3_dbias(void)
{
    int32_t voltage_at_20 = V_DIG_MID_MUL10000 + get_v_dig_dbias20() * 10000 / DBIAS_CALIBRATION_DIVISOR;
    int32_t slope = K_DIG_MID_MUL10000 + get_k_dig_ldo();

    for (uint32_t dbias = DBIAS_MIN; dbias < DBIAS_MAX; ++dbias) {
        int32_t voltage = voltage_at_20 + slope * ((int32_t)dbias - 20);
        if (voltage >= V_DIG_1V3_MUL10000) {
            return dbias;
        }
    }
    return DBIAS_MAX;
}

static uint32_t get_rtc_dbias(uint32_t dig_dbias)
{
    int32_t rtc_voltage_at_20 = V_RTC_MID_MUL10000 + get_v_rtc_dbias20() * 10000 / DBIAS_CALIBRATION_DIVISOR;
    int32_t dig_voltage_at_20 = V_DIG_MID_MUL10000 + get_v_dig_dbias20() * 10000 / DBIAS_CALIBRATION_DIVISOR;
    int32_t rtc_slope = K_RTC_MID_MUL10000 + get_k_rtc_ldo();
    int32_t dig_slope = K_DIG_MID_MUL10000 + get_k_dig_ldo();
    int32_t dig_voltage = dig_voltage_at_20 + dig_slope * ((int32_t)dig_dbias - 20);

    for (uint32_t dbias = DBIAS_MIN; dbias < DBIAS_MAX; ++dbias) {
        int32_t rtc_voltage = rtc_voltage_at_20 + rtc_slope * ((int32_t)dbias - 20);
        if (rtc_voltage >= dig_voltage - DBIAS_RTC_TOLERANCE) {
            return dbias;
        }
    }
    return DBIAS_MAX;
}

static void get_240m_dbias(uint32_t *rtc_dbias, uint32_t *dig_dbias)
{
    *rtc_dbias = RTC_DBIAS_240M_DEFAULT;
    *dig_dbias = DIG_DBIAS_240M_DEFAULT;

    if (!has_pvt_dbias_calibration()) {
        return;
    }

    uint32_t hvt = efuse_field(EFUSE_RD_MAC_SPI_SYS_5_REG, EFUSE_DIG_DBIAS_HVT_V, EFUSE_DIG_DBIAS_HVT_S);
    if (hvt == 0U) {
        return;
    }

    *dig_dbias = MIN(get_dig_1v3_dbias(), hvt + 3U);
    *rtc_dbias = get_rtc_dbias(*dig_dbias);
}

static void set_regulator_dbias(uint32_t rtc_dbias, uint32_t dig_dbias)
{
    esp_rom_regi2c_write_mask(I2C_DIG_REG,
                              I2C_DIG_REG_HOSTID,
                              I2C_DIG_REG_EXT_RTC_DREG,
                              I2C_DIG_REG_EXT_RTC_DREG_MSB,
                              I2C_DIG_REG_EXT_RTC_DREG_LSB,
                              (uint8_t)rtc_dbias);
    esp_rom_regi2c_write_mask(I2C_DIG_REG,
                              I2C_DIG_REG_HOSTID,
                              I2C_DIG_REG_EXT_DIG_DREG,
                              I2C_DIG_REG_EXT_DIG_DREG_MSB,
                              I2C_DIG_REG_EXT_DIG_DREG_LSB,
                              (uint8_t)dig_dbias);
}

static void configure_bbpll(void)
{
    uint8_t div_ref = 0;
    uint8_t div_7_0 = 8;
    uint8_t dr1 = 0;
    uint8_t dr3 = 0;
    uint8_t dchgp = 5;
    uint8_t dcur = 3;

    if (esp_rom_get_xtal_freq() == 32U) {
        div_ref = 1;
        div_7_0 = 26;
        dr1 = 1;
        dr3 = 1;
        dchgp = 4;
        dcur = 0;
    }

    REG_CLR_BIT(RTC_CNTL_OPTIONS0_REG,
                RTC_CNTL_BB_I2C_FORCE_PD | RTC_CNTL_BBPLL_FORCE_PD | RTC_CNTL_BBPLL_I2C_FORCE_PD);
    REG_SET_FIELD(SYSTEM_CPU_PER_CONF_REG, SYSTEM_PLL_FREQ_SEL, 1U);

    REG_CLR_BIT(I2C_MST_ANA_CONF0_REG, I2C_MST_BBPLL_STOP_FORCE_HIGH);
    REG_SET_BIT(I2C_MST_ANA_CONF0_REG, I2C_MST_BBPLL_STOP_FORCE_LOW);

    esp_rom_regi2c_write(I2C_BBPLL, I2C_BBPLL_HOSTID, I2C_BBPLL_MODE_HF, 0x6BU);
    esp_rom_regi2c_write(I2C_BBPLL,
                         I2C_BBPLL_HOSTID,
                         I2C_BBPLL_OC_REF_DIV,
                         (uint8_t)((dchgp << I2C_BBPLL_OC_DCHGP_LSB) | div_ref));
    esp_rom_regi2c_write(I2C_BBPLL, I2C_BBPLL_HOSTID, I2C_BBPLL_OC_DIV_7_0, div_7_0);
    esp_rom_regi2c_write_mask(I2C_BBPLL,
                              I2C_BBPLL_HOSTID,
                              I2C_BBPLL_OC_DR1,
                              I2C_BBPLL_OC_DR1_MSB,
                              I2C_BBPLL_OC_DR1_LSB,
                              dr1);
    esp_rom_regi2c_write_mask(I2C_BBPLL,
                              I2C_BBPLL_HOSTID,
                              I2C_BBPLL_OC_DR3,
                              I2C_BBPLL_OC_DR3_MSB,
                              I2C_BBPLL_OC_DR3_LSB,
                              dr3);
    esp_rom_regi2c_write(I2C_BBPLL,
                         I2C_BBPLL_HOSTID,
                         I2C_BBPLL_OC_DCUR,
                         (uint8_t)((1U << I2C_BBPLL_OC_DLREF_SEL_LSB) | (3U << I2C_BBPLL_OC_DHREF_SEL_LSB) | dcur));
    esp_rom_regi2c_write_mask(I2C_BBPLL,
                              I2C_BBPLL_HOSTID,
                              I2C_BBPLL_OC_VCO_DBIAS,
                              I2C_BBPLL_OC_VCO_DBIAS_MSB,
                              I2C_BBPLL_OC_VCO_DBIAS_LSB,
                              3U);

    while (REG_GET_BIT(I2C_MST_ANA_CONF0_REG, I2C_MST_BBPLL_CAL_DONE) == 0U) {
    }
    stub_lib_delay_us(10);
    REG_CLR_BIT(I2C_MST_ANA_CONF0_REG, I2C_MST_BBPLL_STOP_FORCE_LOW);
    REG_SET_BIT(I2C_MST_ANA_CONF0_REG, I2C_MST_BBPLL_STOP_FORCE_HIGH);
}

void stub_target_clock_init(void)
{
    uint32_t rtc_dbias;
    uint32_t dig_dbias;

    /* Raise the regulator voltage before enabling or switching to the BBPLL. */
    get_240m_dbias(&rtc_dbias, &dig_dbias);
    set_regulator_dbias(rtc_dbias, dig_dbias);
    stub_lib_delay_us(DBIAS_SETTLE_US);

    if (REG_GET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_SOC_CLK_SEL) != 1U) {
        configure_bbpll();
    }

    /* At 240 MHz all six LDO slaves remain enabled (no slave power-down bits). */
    REG_SET_FIELD(RTC_CNTL_DATE_REG, RTC_CNTL_SLAVE_PD, DEFAULT_LDO_SLAVE >> (CPU_FREQ_MHZ / 80U));

    s_cpu_freq = CPU_FREQ_MHZ * MHZ;

    REG_SET_FIELD(SYSTEM_CPU_PER_CONF_REG, SYSTEM_PLL_FREQ_SEL, 1U);
    REG_SET_FIELD(SYSTEM_CPU_PER_CONF_REG, SYSTEM_CPUPERIOD_SEL, 2U);
    REG_SET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_PRE_DIV_CNT, 0U);
    REG_SET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_SOC_CLK_SEL, 1U);
    esp_rom_set_cpu_ticks_per_us(CPU_FREQ_MHZ);
}

uint32_t stub_target_get_cpu_freq(void)
{
    if (s_cpu_freq == 0) {
        return esp_rom_get_cpu_ticks_per_us() * MHZ;
    }
    return s_cpu_freq;
}

uint32_t stub_target_get_apb_freq(void)
{
    uint32_t clock;
    uint32_t clock_sel = REG_GET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_SOC_CLK_SEL);
    if (clock_sel == 0) { // from xtal, 80MHz, 40MHz, 20MHz, 10MHz, 8MHz,...
        // Should be also divided by SYSTEM_PRE_DIV_CNT, but setting divider has no effect on ESP32S3.
        clock = esp_rom_get_xtal_freq();
    } else if (clock_sel == 1) { // from pll, 80MHz
        clock = 80 * MHZ;
    } else if (clock_sel == 2) { // 8M RC, about 8MHz, code will not come here
        clock = 8 * MHZ;
    } else { // audio pll, code will not come here, just put an different clock here.
        clock = 16 * MHZ;
    }
    return clock;
}

#define RTC_CNTL_WDT_KEY 0x50D83AA1
#define RTC_CNTL_SWD_KEY 0x8F1D312A

void stub_target_clock_disable_watchdogs(void)
{
    // Disable RWDT (RTC Watchdog)
    REG_SET_BIT(RTC_CNTL_INT_CLR_REG, RTC_CNTL_WDT_INT_CLR);
    WRITE_PERI_REG(RTC_CNTL_WDTWPROTECT_REG, RTC_CNTL_WDT_KEY);
    WRITE_PERI_REG(RTC_CNTL_WDTCONFIG0_REG, 0x0);
    WRITE_PERI_REG(RTC_CNTL_WDTWPROTECT_REG, 0x0);

    // Configure SWD (Super Watchdog) to autofeed
    REG_SET_BIT(RTC_CNTL_INT_CLR_REG, RTC_CNTL_SWD_INT_CLR);
    WRITE_PERI_REG(RTC_CNTL_SWD_WPROTECT_REG, RTC_CNTL_SWD_KEY);
    SET_PERI_REG_MASK(RTC_CNTL_SWD_CONF_REG, RTC_CNTL_SWD_AUTO_FEED_EN);
    WRITE_PERI_REG(RTC_CNTL_SWD_WPROTECT_REG, 0x0);
}
