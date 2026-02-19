/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <target/clock.h>
#include <soc_utils.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/system_reg.h>
#include <soc/timer_group_reg.h>

extern void esp_rom_set_cpu_ticks_per_us(uint32_t ticks_per_us);
extern uint32_t esp_rom_get_cpu_ticks_per_us(void);
extern void esp_rom_delay_us(uint32_t us);

/*
 * The TIMG0 calibration unit measures XTAL frequency by counting XTAL clock
 * edges that occur during N_SLOW_CYCLES cycles of the RC_FAST/256 clock:
 *
 *   xtal_hz = cal_val * (RC_FAST_HZ / 256) / N_SLOW_CYCLES
 *
 * RC_FAST_HZ is the nominal value (17.5 MHz). The actual per-chip frequency
 * varies with silicon and can reach ~20 MHz, causing xtal_mhz to read ~12%
 * lower than the true value (e.g. 40 MHz crystal measures as ~35 MHz).
 * Snapping to the correct discrete ESP32-C2 XTAL option (26 or 40 MHz)
 * tolerates this error as long as the measurement stays on the correct side
 * of the midpoint (33 MHz).
 *
 * N_SLOW_CYCLES = 10 gives a ~128 µs measurement window at 20 MHz RC_FAST.
 * TIMEOUT_THRES = N_SLOW_CYCLES << 12 = 40960 XTAL cycles ≈ 1 ms at 40 MHz,
 * roughly 7× the expected window — ample margin before declaring failure.
 */
#define RC_FAST_HZ    17500000UL
#define N_SLOW_CYCLES 10u
#define TIMEOUT_THRES (N_SLOW_CYCLES << 12)

static uint32_t s_cpu_freq = 0;
static uint32_t s_crystal_freq = 0;

/*
 * Estimate the XTAL frequency using the TIMG0 hardware calibration unit.
 * Returns 26 or 40 MHz in Hz. Falls back to 40 MHz on hardware timeout.
 */
static uint32_t detected_crystal_freq(void)
{
    /* Enable TIMG0 peripheral clock and release it from reset. */
    SET_PERI_REG_MASK(SYSTEM_PERIP_CLK_EN0_REG, SYSTEM_TIMERGROUP_CLK_EN);
    CLEAR_PERI_REG_MASK(SYSTEM_PERIP_RST_EN0_REG, SYSTEM_TIMERGROUP_RST);

    /* Snapshot the clock config so we can restore it afterwards. */
    uint32_t saved_clk_conf = READ_PERI_REG(RTC_CNTL_CLK_CONF_REG);
    int rc_fast_was_on = !(saved_clk_conf & RTC_CNTL_ENB_CK8M);
    int d256_was_on = !(saved_clk_conf & RTC_CNTL_ENB_CK8M_DIV);
    int d256_digi_was = (saved_clk_conf & RTC_CNTL_DIG_CLK8M_D256_EN);

    /* Enable RC_FAST oscillator and its /256 divider, then wait for the
     * oscillator to stabilise (datasheet recommends ≥50 µs). CK8M_WAIT=5
     * mirrors the value IDF uses in clk_ll_rc_fast_enable(). */
    CLEAR_PERI_REG_MASK(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_ENB_CK8M);
    REG_SET_FIELD(RTC_CNTL_TIMER1_REG, RTC_CNTL_CK8M_WAIT, 5u);
    CLEAR_PERI_REG_MASK(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_ENB_CK8M_DIV);
    esp_rom_delay_us(50);

    /* Route the RC_FAST/256 signal onto the digital bus that feeds TIMG. */
    SET_PERI_REG_MASK(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_DIG_CLK8M_D256_EN);

    /* If cycling calibration is already running, force a quick timeout so
     * we can safely reconfigure the calibration registers. */
    if (GET_PERI_REG_MASK(TIMG_RTCCALICFG_REG(0), TIMG_RTC_CALI_START_CYCLING)) {
        REG_SET_FIELD(TIMG_RTCCALICFG2_REG(0), TIMG_RTC_CALI_TIMEOUT_THRES, 1u);
        while (!GET_PERI_REG_MASK(TIMG_RTCCALICFG_REG(0), TIMG_RTC_CALI_RDY) &&
               !GET_PERI_REG_MASK(TIMG_RTCCALICFG2_REG(0), TIMG_RTC_CALI_TIMEOUT))
            ;
    }

    /* Configure one-shot calibration: source = RC_FAST/256 (CLK_SEL=1),
     * count N_SLOW_CYCLES slow-clock cycles, set the timeout guard. */
    REG_SET_FIELD(TIMG_RTCCALICFG_REG(0), TIMG_RTC_CALI_CLK_SEL, 1u);
    CLEAR_PERI_REG_MASK(TIMG_RTCCALICFG_REG(0), TIMG_RTC_CALI_START_CYCLING);
    REG_SET_FIELD(TIMG_RTCCALICFG_REG(0), TIMG_RTC_CALI_MAX, N_SLOW_CYCLES);
    REG_SET_FIELD(TIMG_RTCCALICFG2_REG(0), TIMG_RTC_CALI_TIMEOUT_THRES, TIMEOUT_THRES);

    /* Rising edge on CALI_START launches the measurement and simultaneously
     * clears the CALI_RDY and CALI_TIMEOUT flags from any previous run. */
    CLEAR_PERI_REG_MASK(TIMG_RTCCALICFG_REG(0), TIMG_RTC_CALI_START);
    SET_PERI_REG_MASK(TIMG_RTCCALICFG_REG(0), TIMG_RTC_CALI_START);

    uint32_t cal_val = 0;
    while (1) {
        if (GET_PERI_REG_MASK(TIMG_RTCCALICFG_REG(0), TIMG_RTC_CALI_RDY)) {
            cal_val = REG_GET_FIELD(TIMG_RTCCALICFG1_REG(0), TIMG_RTC_CALI_VALUE);
            break;
        }
        if (GET_PERI_REG_MASK(TIMG_RTCCALICFG2_REG(0), TIMG_RTC_CALI_TIMEOUT)) {
            break;
        }
    }
    CLEAR_PERI_REG_MASK(TIMG_RTCCALICFG_REG(0), TIMG_RTC_CALI_START);

    /* Restore clock config to whatever it was before we touched it. */
    if (!d256_digi_was) {
        CLEAR_PERI_REG_MASK(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_DIG_CLK8M_D256_EN);
    }
    if (!rc_fast_was_on) {
        SET_PERI_REG_MASK(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_ENB_CK8M);
        REG_SET_FIELD(RTC_CNTL_TIMER1_REG, RTC_CNTL_CK8M_WAIT, 20u);
    }
    if (!d256_was_on) {
        SET_PERI_REG_MASK(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_ENB_CK8M_DIV);
    }

    if (cal_val == 0) {
        return 40 * MHZ;
    }

    /* Convert the raw edge count to MHz. The 64-bit intermediate prevents
     * overflow (cal_val × RC_FAST_HZ can exceed 2^32 at the values involved). */
    uint32_t xtal_mhz = (uint32_t)((uint64_t)cal_val * RC_FAST_HZ / (256UL * N_SLOW_CYCLES * 1000000UL));

    /* Snap to 26 or 40 MHz — the only two crystals used on ESP32-C2.
     * Threshold is the midpoint (33 MHz). Even with RC_FAST running ~14%
     * above nominal, a 26 MHz crystal measures ~22 MHz and a 40 MHz crystal
     * measures ~35 MHz, both comfortably on the correct side of 33 MHz. */
    return (xtal_mhz < 33) ? 26 * MHZ : 40 * MHZ;
}

void stub_target_clock_init(void)
{
    // This enables PLL which multiplies the crystal frequency by 12/6 = 2x.
    REG_SET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_SOC_CLK_SEL, 1);
    REG_SET_FIELD(SYSTEM_CPU_PER_CONF_REG, SYSTEM_PLL_FREQ_SEL, 0);

    /*
    TODO: Increase CPU frequency to at least 80 MHz for 26 MHz crystal.
    */
    s_crystal_freq = detected_crystal_freq();
    s_cpu_freq = s_crystal_freq * 2;
    esp_rom_set_cpu_ticks_per_us(s_cpu_freq / MHZ);
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
    // APB frequency is the same as crystal frequency.
    if (s_crystal_freq == 0) {
        s_crystal_freq = detected_crystal_freq();
    }
    return s_crystal_freq;
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
