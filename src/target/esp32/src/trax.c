/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <soc_utils.h>

#include "soc/reg_base.h"

#define TRACEMEM_MUX_MODE_REG           (DR_REG_DPORT_BASE + 0x070)
#define TRACEMEM_ENA_REG                (DR_REG_DPORT_BASE + 0x074)
#define TRACEMEM_ENA_M                  (0x1)

#define TRACEMEM_MUX_BLK0_ONLY          1
#define TRACEMEM_MUX_BLK1_ONLY          2

void stub_target_trax_mem_enable(void)
{
    WRITE_PERI_REG(TRACEMEM_ENA_REG, TRACEMEM_ENA_M);
}

void stub_target_trax_select_mem_block(int block)
{
    WRITE_PERI_REG(TRACEMEM_MUX_MODE_REG, block ? TRACEMEM_MUX_BLK0_ONLY : TRACEMEM_MUX_BLK1_ONLY);
}
