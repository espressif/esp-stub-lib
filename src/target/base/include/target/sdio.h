/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool stub_target_sdio_is_active(void);

void stub_target_sdio_init(void);

bool stub_target_sdio_take_rx_frame(size_t *out_len);

int stub_target_sdio_rearm(uint8_t *buf, size_t max_size);

int stub_target_sdio_tx_frame(const void *data, size_t len);

#ifdef __cplusplus
}
#endif
