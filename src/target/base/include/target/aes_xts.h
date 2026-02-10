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

/* Maximum size of an encryption block */
#define MAX_ENCRYPT_BLOCK 64

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
 * @brief Initialize AES-XTS
 *
 * This function sets the destination register to flash (0 = flash, 1 = PSRAM).
 *
 * Note: Caller is responsible for enabling encryption at ROM level and
 * unlocking the flash before calling this function.
 */
void stub_target_aes_xts_init(void);

/**
 * @brief Prepare and trigger encryption for a data block
 *
 * This function:
 * - Sets the block size in the AES-XTS engine
 * - Copies plaintext data to the encryption buffer
 * - Sets the target address
 * - Triggers the encryption operation
 *
 * After calling this function, the caller should:
 * 1. Call stub_target_aes_xts_wait_data_ready() to wait for encryption and release
 * 2. Write the encrypted data
 * 3. Destroy the encryption buffer
 *
 * @param flash_addr Target address (must be 16-byte aligned)
 * @param data Plaintext data to encrypt
 * @param block_size Size of data block (16, 32, or 64 bytes)
 */
void stub_target_aes_xts_encrypt_trigger(uint32_t flash_addr, const void *data, uint32_t block_size);

/**
 * @brief Wait for encrypted data to be ready and release it to MSPI
 *
 * This function:
 * 1. Waits for encryption state to become 0x2 (done but invisible)
 * 2. Releases the encrypted data by writing to AES_XTS_RELEASE_REG
 * 3. Waits for state to become 0x3 (visible to mspi)
 *
 * @param timeout_us Maximum number of microseconds to wait
 * @return STUB_LIB_OK on success, STUB_LIB_ERR_TIMEOUT on timeout
 */
int stub_target_aes_xts_wait_data_ready(uint64_t timeout_us);

/**
 * @brief Destroy/clear the encryption buffer
 *
 * This should be called after each encrypted block is successfully written
 * to clear the encryption buffer and prepare for the next block.
 */
void stub_target_aes_xts_clear(void);
