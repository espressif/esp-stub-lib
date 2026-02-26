/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <stddef.h>
#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/sha.h>

#include <soc/reg_base.h>

#define SHA_TEXT_BASE        (DR_REG_SHA_BASE)
#define SHA_256_START_REG    (DR_REG_SHA_BASE + 0x90)
#define SHA_256_CONTINUE_REG (DR_REG_SHA_BASE + 0x94)
#define SHA_256_LOAD_REG     (DR_REG_SHA_BASE + 0x98)
#define SHA_256_BUSY_REG     (DR_REG_SHA_BASE + 0x9c)

#define BLOCK_WORDS          (64 / sizeof(uint32_t))
#define DIGEST_WORDS         (32 / sizeof(uint32_t))

extern void ets_sha_enable(void);
extern void ets_sha_disable(void);

static uint32_t words_hashed;

void stub_target_sha256_start(void)
{
    words_hashed = 0;
    ets_sha_enable();
}

void stub_target_sha256_data(const void *data, uint32_t data_len)
{
    if (data_len % 4 != 0) {
        return;
    }

    const uint32_t *w = (const uint32_t *)data;
    size_t word_len = data_len / 4;
    volatile uint32_t *sha_text_reg = (volatile uint32_t *)SHA_TEXT_BASE;

    while (word_len > 0) {
        size_t block_count = words_hashed % BLOCK_WORDS;
        size_t copy_words = BLOCK_WORDS - block_count;
        copy_words = MIN(word_len, copy_words);

        /* Wait for SHA engine idle */
        while (REG_READ(SHA_256_BUSY_REG) != 0) {
            /* busy wait */
        }

        /* Copy to memory block */
        for (size_t i = 0; i < copy_words; i++) {
            sha_text_reg[block_count + i] = __builtin_bswap32(w[i]);
        }
        asm volatile("memw");

        /* Update counters */
        words_hashed += copy_words;
        block_count += copy_words;
        word_len -= copy_words;
        w += copy_words;

        /* If we loaded a full block, run the SHA engine */
        if (block_count == BLOCK_WORDS) {
            if (words_hashed == BLOCK_WORDS) {
                REG_WRITE(SHA_256_START_REG, 1);
            } else {
                REG_WRITE(SHA_256_CONTINUE_REG, 1);
            }
        }
    }
}

void stub_target_sha256_finish(uint8_t *digest)
{
    if (!digest) {
        ets_sha_disable();
        return;
    }

    uint32_t data_words = words_hashed;

    /*
     * Pad to a 55-byte-long block loaded in the engine
     * (1 byte 0x80 + variable padding + 8 bytes of length = 64-byte block).
     */
    int block_bytes = (int)((words_hashed % BLOCK_WORDS) * 4);
    int pad_bytes = 55 - block_bytes;
    if (pad_bytes < 0) {
        pad_bytes += 64;
    }
    static const uint8_t padding[64] = {
        0x80,
        0,
    };

    /* 1 byte for 0x80 plus first 4 bytes of the 64-bit length */
    pad_bytes += 5;

    stub_target_sha256_data(padding, (size_t)pad_bytes);

    uint32_t bit_count = __builtin_bswap32(data_words * 32);
    stub_target_sha256_data(&bit_count, sizeof(bit_count));

    while (REG_READ(SHA_256_BUSY_REG) == 1) {
        /* busy wait */
    }
    REG_WRITE(SHA_256_LOAD_REG, 1);
    while (REG_READ(SHA_256_BUSY_REG) == 1) {
        /* busy wait */
    }

    uint32_t *digest_words = (uint32_t *)digest;
    volatile uint32_t *sha_text_reg = (volatile uint32_t *)SHA_TEXT_BASE;
    for (size_t i = 0; i < DIGEST_WORDS; i++) {
        digest_words[i] = __builtin_bswap32(sha_text_reg[i]);
    }
    asm volatile("memw");

    ets_sha_disable();
}
