/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <esp-stub-lib/soc_utils.h>
#include <esp-stub-lib/err.h>
#include <esp-stub-lib/rom_wrappers.h>

/**
 * @brief Wait for a register to reach an expected state
 *
 * Polls a register until it matches the expected state or times out.
 * This is a general-purpose utility for polling register state transitions.
 *
 * @param reg Register address to poll
 * @param expected_state Expected register value
 * @param timeout_us Pointer to timeout in microseconds (decremented on each iteration)
 * @return STUB_LIB_OK on success, STUB_LIB_ERR_TIMEOUT on timeout
 */
static inline int stub_target_wait_reg_state(uint32_t reg, uint32_t expected_state, uint64_t *timeout_us)
{
    if (!timeout_us || *timeout_us == 0) {
        return REG_READ(reg) == expected_state ? STUB_LIB_OK : STUB_LIB_ERR_TIMEOUT;
    }

    while ((*timeout_us)--) {
        stub_lib_delay_us(1);
        if (REG_READ(reg) == expected_state) {
            return STUB_LIB_OK;
        }
    }
    return STUB_LIB_ERR_TIMEOUT;
}

/**
 * @brief Wait for a register bit mask to be set
 *
 * Polls a register until the specified bits are set or times out.
 * Useful for checking hardware status flags.
 *
 * @param reg Register address to poll
 * @param bit_mask Bit mask to check (bits that should be set)
 * @param timeout_us Pointer to timeout in microseconds (decremented on each iteration)
 * @return STUB_LIB_OK on success, STUB_LIB_ERR_TIMEOUT on timeout
 */
static inline int stub_target_wait_reg_bit_set(uint32_t reg, uint32_t bit_mask, uint64_t *timeout_us)
{
    if (!timeout_us || *timeout_us == 0) {
        return (REG_READ(reg) & bit_mask) ? STUB_LIB_OK : STUB_LIB_ERR_TIMEOUT;
    }

    while ((*timeout_us)--) {
        stub_lib_delay_us(1);
        if (REG_READ(reg) & bit_mask) {
            return STUB_LIB_OK;
        }
    }
    return STUB_LIB_ERR_TIMEOUT;
}
