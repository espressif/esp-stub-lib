/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <target/cache.h>

extern void Cache_WriteBack_Invalidate_All(void);
extern void Cache_Invalidate_All(void);

void stub_target_cache_writeback_invalidate_all(void)
{
    Cache_WriteBack_Invalidate_All();
}

void stub_target_cache_invalidate_all(void)
{
    Cache_Invalidate_All();
}
