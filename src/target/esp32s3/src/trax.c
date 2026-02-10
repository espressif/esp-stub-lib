/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <bit_utils.h>
#include <soc_utils.h>

#include "soc/reg_base.h"

#define SENSITIVE_INTERNAL_SRAM_USAGE_2_REG (DR_REG_SENSITIVE_BASE + 0x18)

#define TRACEMEM_MUX_BLK0_NUM               22
#define TRACEMEM_MUX_BLK1_NUM               26

#define TRACEMEM_MUX_BLK_ALLOC(_n_)         (((_n_) - 2UL) % 4UL)
#define TRACEMEM_CORE0_MUX_BLK_BITS(_n_)    (BIT(((_n_) - 2UL) / 4UL) | (TRACEMEM_MUX_BLK_ALLOC(_n_) << 14))
#define TRACEMEM_CORE1_MUX_BLK_BITS(_n_)    (BIT(7UL + ((_n_) - 2UL) / 4UL) | (TRACEMEM_MUX_BLK_ALLOC(_n_) << 16))

void stub_target_trax_mem_enable(void)
{ /* nothing to do for ESP32S3 */
}

void stub_target_trax_select_mem_block(int block)
{
    uint32_t block_bits =
        block ? TRACEMEM_CORE0_MUX_BLK_BITS(TRACEMEM_MUX_BLK0_NUM) : TRACEMEM_CORE0_MUX_BLK_BITS(TRACEMEM_MUX_BLK1_NUM);
    block_bits |=
        block ? TRACEMEM_CORE1_MUX_BLK_BITS(TRACEMEM_MUX_BLK0_NUM) : TRACEMEM_CORE1_MUX_BLK_BITS(TRACEMEM_MUX_BLK1_NUM);
    WRITE_PERI_REG(SENSITIVE_INTERNAL_SRAM_USAGE_2_REG, block_bits);
}
