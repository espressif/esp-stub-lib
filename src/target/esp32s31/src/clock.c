/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <esp-stub-lib/soc_utils.h>

#include <target/clock.h>

#include <soc/hp_sys_clkrst_reg.h>
#include <soc/pmu_reg.h>
#include <soc/soc.h>

#define HP_CALI_DBIAS_DEFAULT     26U
#define SOC_CPU_CLK_SRC_PLL_F240M 3U

extern uint32_t esp_rom_get_cpu_freq(void);
extern void esp_rom_set_cpu_ticks_per_us(uint32_t ticks_per_us);

static uint32_t s_cpu_freq;

static void update_bus_clocks(void)
{
    REG_SET_BIT(HP_SYS_CLKRST_ROOT_CLK_CTRL0_REG, HP_SYS_CLKRST_REG_SOC_CLK_UPDATE);
    while (REG_READ(HP_SYS_CLKRST_ROOT_CLK_CTRL0_REG) & HP_SYS_CLKRST_REG_SOC_CLK_UPDATE) {
    }
}

void stub_target_clock_init(void)
{
    /*
     * S31 ROM leaves BBPLL at 480 MHz and its digital gate enabled. Use the
     * existing divided 240 MHz output instead of recalibrating a PLL.
     */
    REG_SET_FIELD(PMU_HP_ACTIVE_HP_REGULATOR0_REG, PMU_HP_ACTIVE_HP_REGULATOR_DBIAS, HP_CALI_DBIAS_DEFAULT);

    /* Match 240 MHz configuration: CPU 240, MEM 120, SYS 80, APB 40 MHz. */
    REG_SET_FIELD(HP_SYS_CLKRST_CPU_FREQ_CTRL0_REG, HP_SYS_CLKRST_REG_CPU_CLK_DIV_NUM, 0U);
    REG_SET_FIELD(HP_SYS_CLKRST_MEM_FREQ_CTRL0_REG, HP_SYS_CLKRST_REG_MEM_CLK_DIV_NUM, 1U);
    REG_SET_FIELD(HP_SYS_CLKRST_SYS_FREQ_CTRL0_REG, HP_SYS_CLKRST_REG_SYS_CLK_DIV_NUM, 2U);
    REG_SET_FIELD(HP_SYS_CLKRST_APB_FREQ_CTRL0_REG, HP_SYS_CLKRST_REG_APB_CLK_DIV_NUM, 1U);
    REG_SET_FIELD(HP_SYS_CLKRST_SOC_CLK_SEL_REG, HP_SYS_CLKRST_REG_SOC_CLK_SEL, SOC_CPU_CLK_SRC_PLL_F240M);

    update_bus_clocks();
    s_cpu_freq = esp_rom_get_cpu_freq();
    esp_rom_set_cpu_ticks_per_us(s_cpu_freq / MHZ);
}

uint32_t stub_target_get_cpu_freq(void)
{
    return s_cpu_freq ? s_cpu_freq : esp_rom_get_cpu_freq();
}

uint32_t stub_target_get_apb_freq(void)
{
    uint32_t cpu_div = REG_GET_FIELD(HP_SYS_CLKRST_CPU_FREQ_CTRL0_REG, HP_SYS_CLKRST_REG_CPU_CLK_DIV_NUM) + 1U;
    uint32_t sys_div = REG_GET_FIELD(HP_SYS_CLKRST_SYS_FREQ_CTRL0_REG, HP_SYS_CLKRST_REG_SYS_CLK_DIV_NUM) + 1U;
    uint32_t apb_div = REG_GET_FIELD(HP_SYS_CLKRST_APB_FREQ_CTRL0_REG, HP_SYS_CLKRST_REG_APB_CLK_DIV_NUM) + 1U;

    return stub_target_get_cpu_freq() * cpu_div / sys_div / apb_div;
}
