/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <stdint.h>

int32_t __bswapsi2(int32_t u)
{
    return (int32_t)(((((uint32_t)u) & 0xff000000) >> 24)
                     | ((((uint32_t)u) & 0x00ff0000) >>  8)
                     | ((((uint32_t)u) & 0x0000ff00) <<  8)
                     | ((((uint32_t)u) & 0x000000ff) << 24));
}

int64_t __ashldi3(int64_t a, int b)
{
    const int bits_in_word = 32;

    // Union to access high and low 32-bit words
    union {
        int64_t all;
        struct {
            uint32_t low;
            uint32_t high;
        } s;
    } input, result;

    input.all = a;

    if (b == 0) {
        return a;
    }

    if (b >= 64) {
        return 0;
    }

    if (b >= bits_in_word) {
        // Shift >= 32: move low word to high, zero out low
        result.s.low = 0;
        result.s.high = input.s.low << (b - bits_in_word);
    } else {
        // Shift < 32: shift both words, carry bits from low to high
        result.s.low = input.s.low << b;
        result.s.high = (input.s.high << b) | (input.s.low >> (bits_in_word - b));
    }

    return result.all;
}

// TODO: It seems esp8266 don't have a full set of the compiler runtime in its ROM.
// If we face strange linker errors like "undefined reference to `__bswapsi2'"
// we may need to provide implementations here
