/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>

#include <bit_utils.h>
#include <soc_utils.h>

#include "soc/reg_base.h"

#define PMS_OCCUPY_3_REG                (DR_REG_SENSITIVE_BASE + 0x0E0)

#define TRACEMEM_MUX_BLK0_NUM           19
#define TRACEMEM_MUX_BLK1_NUM           20

void stub_target_trax_mem_enable(void)
{
    /* nothing to do for ESP32S2 */
}

void stub_target_trax_select_mem_block(int block)
{
    WRITE_PERI_REG(PMS_OCCUPY_3_REG, block ? BIT(TRACEMEM_MUX_BLK0_NUM - 4) : BIT(TRACEMEM_MUX_BLK1_NUM - 4));
}
