/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>

#include <soc/soc_caps.h>

/*
 * Espressif performance counter CSRs, named as in esp-rom riscv/csr.h. The ROM start code sets them on chips
 * with SUPPORT_RV_CSR_PC (SOC_CPU_HAS_CSR_PC in ESP-IDF), and ets_delay_us() reads the count through the
 * user-mode alias of mpccr (0x802). The counter is stopped after a CPU reset.
 */
#define CSR_PCER_M     0x7e0  /* mpcer, selects which events are counted */
#define CSR_PCMR_M     0x7e1  /* mpcmr, controls the counter mode */
#define PCER_CYCLES    BIT(0) /* Event select bit that counts clock cycles */
#define PCMR_GLOBAL_EN BIT(0) /* Enables counting */

__attribute__((weak)) void stub_target_clock_init(void)
{
}

__attribute__((weak)) uint32_t stub_target_get_cpu_freq(void)
{
    return 0;
}

__attribute__((weak)) uint32_t stub_target_get_apb_freq(void)
{
    return 0;
}

__attribute__((weak)) void stub_target_clock_disable_watchdogs(void)
{
}

__attribute__((weak)) void stub_target_clock_enable_perf_counter(void)
{
#if defined(SOC_CPU_HAS_CSR_PC) && SOC_CPU_HAS_CSR_PC
    __asm__ volatile("csrw %0, %1" ::"i"(CSR_PCER_M), "r"(PCER_CYCLES));
    __asm__ volatile("csrw %0, %1" ::"i"(CSR_PCMR_M), "r"(PCMR_GLOBAL_EN));
#endif
}
