/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <private/helpers.h>
#include <target/aes_xts.h>
#include <esp-stub-lib/soc_utils.h>
#include <esp-stub-lib/err.h>
#include <esp-stub-lib/rom_wrappers.h>
#include <soc/hwcrypto_reg.h>

/* AES-XTS state register values:
 * 0: idle
 * 1: busy of encryption calculation
 * 2: encryption calculation is done but the encrypted result is invisible to mspi
 * 3: the encrypted result is visible to mspi
 */
#define AES_XTS_STATE_IDLE    0x0
#define AES_XTS_STATE_BUSY    0x1
#define AES_XTS_STATE_DONE    0x2
#define AES_XTS_STATE_VISIBLE 0x3

extern void *memcpy(void *dest, const void *src, size_t n);

void stub_target_aes_xts_init(void)
{
    /* Set destination to flash (0 = flash, 1 = PSRAM) */
    REG_WRITE(AES_XTS_DESTINATION_REG, 0);
}

void stub_target_aes_xts_encrypt_trigger(uint32_t flash_addr, const void *data, uint32_t block_size)
{
    /* Calculate plaintext offset within the AES-XTS buffer */
    uint32_t plaintext_offs = (flash_addr % MAX_ENCRYPT_BLOCK);

    /* Set block size: 0 for 16 bytes, 1 for 32 bytes, 2 for 64 bytes */
    REG_WRITE(AES_XTS_SIZE_REG, block_size >> 5);

    /* Copy plaintext data to AES-XTS input buffer */
    memcpy((void *)(AES_XTS_PLAIN_BASE + plaintext_offs), data, block_size);

    /* Set the physical address for encryption */
    REG_WRITE(AES_XTS_PHYSICAL_ADDR_REG, flash_addr);

    /* Trigger the encryption operation */
    REG_WRITE(AES_XTS_TRIGGER_REG, 1);
}

int stub_target_aes_xts_wait_data_ready(uint64_t timeout_us)
{
    /* Wait for encryption to complete (state becomes 2: done but invisible) */
    int ret = stub_target_wait_reg_state(AES_XTS_STATE_REG, AES_XTS_STATE_DONE, &timeout_us);
    if (ret != STUB_LIB_OK) {
        return ret;
    }

    /* Release encrypted data to make it visible to mspi */
    REG_WRITE(AES_XTS_RELEASE_REG, 1);

    /* Wait for data to become visible (state becomes 3: visible to mspi) */
    return stub_target_wait_reg_state(AES_XTS_STATE_REG, AES_XTS_STATE_VISIBLE, &timeout_us);
}

void stub_target_aes_xts_clear(void)
{
    /* Destroy/clear the encryption buffer after write is complete */
    REG_WRITE(AES_XTS_DESTROY_REG, 1);
}
