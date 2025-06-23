/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#define ETS_UNCACHED_ADDR(addr) (addr)
#define ETS_CACHED_ADDR(addr) (addr)

// Base register read/write macros
#define REG_READ(_r) ({                                                                                                \
            (*(volatile uint32_t *)(_r));                                                                              \
        })

#define REG_WRITE(_r, _v)  do {                                                                                        \
            (*(volatile uint32_t *)(_r)) = (_v);                                                                       \
        } while(0)

// Bit manipulation macros
#define REG_GET_BIT(_r, _b)  ({                                                                                        \
            (REG_READ(_r) & (_b));                                                                                     \
        })

#define REG_SET_BIT(_r, _b)  do {                                                                                      \
            REG_WRITE(_r, REG_READ(_r) | (_b));                                                                        \
        } while(0)

#define REG_CLR_BIT(_r, _b)  do {                                                                                      \
            REG_WRITE(_r, REG_READ(_r) & (~(_b)));                                                                     \
        } while(0)

#define REG_SET_BITS(_r, _b, _m) do {                                                                                  \
            REG_WRITE(_r, (REG_READ(_r) & ~(_m)) | ((_b) & (_m)));                                                     \
        } while(0)

// Field manipulation macros
#define REG_GET_FIELD(_r, _f) ({                                                                                       \
            ((REG_READ(_r) >> (_f##_S)) & (_f##_V));                                                                   \
        })

#define REG_SET_FIELD(_r, _f, _v) do {                                                                                 \
            REG_WRITE(_r, (REG_READ(_r) & ~((_f##_V) << (_f##_S))) | (((_v) & (_f##_V)) << (_f##_S)));                 \
        } while(0)

// Value field manipulation macros
#define VALUE_GET_FIELD(_r, _f) (((_r) >> (_f##_S)) & (_f))

#define VALUE_GET_FIELD2(_r, _f) (((_r) & (_f)) >> (_f##_S))

#define VALUE_SET_FIELD(_r, _f, _v) ((_r) = ((_r) & ~((_f) << (_f##_S))) | ((_v) << (_f##_S)))

#define VALUE_SET_FIELD2(_r, _f, _v) ((_r) = ((_r) & ~(_f)) | ((_v) << (_f##_S)))

// Field to value conversion macros
#define FIELD_TO_VALUE(_f, _v) (((_v) & (_f)) << _f##_S)

#define FIELD_TO_VALUE2(_f, _v) (((_v) << _f##_S) & (_f))

// Peripheral register macros
#define READ_PERI_REG(addr) REG_READ(ETS_UNCACHED_ADDR(addr))

#define WRITE_PERI_REG(addr, val) REG_WRITE(ETS_UNCACHED_ADDR(addr), (uint32_t)(val))

#define CLEAR_PERI_REG_MASK(reg, mask) do {                                                                            \
            WRITE_PERI_REG(reg, READ_PERI_REG(reg) & (~(mask)));                                                       \
        } while(0)

#define SET_PERI_REG_MASK(reg, mask) do {                                                                              \
            WRITE_PERI_REG(reg, READ_PERI_REG(reg) | (mask));                                                          \
        } while(0)

#define GET_PERI_REG_MASK(reg, mask) ({                                                                                \
            (READ_PERI_REG(reg) & (mask));                                                                             \
        })

#define GET_PERI_REG_BITS(reg, hipos, lowpos) ({                                                                       \
            ((READ_PERI_REG(reg) >> (lowpos)) & ((1 << ((hipos) - (lowpos) + 1)) - 1));                                \
        })

#define SET_PERI_REG_BITS(reg, bit_map, value, shift) do {                                                             \
            WRITE_PERI_REG(reg, (READ_PERI_REG(reg) & (~((bit_map) << (shift)))) | (((value) & (bit_map)) << (shift))); \
        } while(0)

#define GET_PERI_REG_BITS2(reg, mask, shift) ({                                                                        \
            ((READ_PERI_REG(reg) >> (shift)) & (mask));                                                                \
        })
