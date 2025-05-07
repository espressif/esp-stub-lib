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

// TODO: It seems esp8266 don't have a full set of the compiler runtime in its ROM.
// If we face strange linker errors like "undefined reference to `__bswapsi2'"
// we may need to provide implementations here
