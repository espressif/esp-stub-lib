/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BIT
#define BIT(nr)                 (1UL << (nr))
#endif

#ifndef BIT64
#define BIT64(nr)               (1ULL << (nr))
#endif

#ifndef ALIGN_MASK
#define ALIGN_MASK(x, mask)             \
({                                      \
    typeof(mask) _mask = (mask);        \
    ((x) + _mask) & ~_mask;             \
})
#endif

#ifndef ALIGN_UP
#define ALIGN_UP(x, a)          ALIGN_MASK(x, (typeof(x))(a) - 1)
#endif

#ifndef ALIGN_DOWN
#define ALIGN_DOWN(x, a)        ((x) & ~((typeof(x))(a) - 1))
#endif

#ifndef IS_ALIGNED
#define IS_ALIGNED(x, a)        (((x) & ((typeof(x))(a) - 1)) == 0)
#endif

#ifndef MIN
#define MIN(a, b)               ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b)               ((a) > (b) ? (a) : (b))
#endif

#ifdef __cplusplus
}
#endif
