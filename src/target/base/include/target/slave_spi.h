/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <soc/soc_caps.h>

/** Required DMA buffer alignment in bytes.
 *  Derived from SOC_DMA_BUF_ALIGN (set in soc_caps.h for cached targets),
 *  defaults to 4 (DMA word alignment) otherwise. */
#ifdef SOC_DMA_BUF_ALIGN
#define SLAVE_SPI_DMA_ALIGN SOC_DMA_BUF_ALIGN
#else
#define SLAVE_SPI_DMA_ALIGN 4U
#endif

#ifdef __cplusplus
extern "C" {
#endif

bool stub_target_slave_spi_is_active(void);
void stub_target_slave_spi_init(void);

bool stub_target_slave_spi_take_rx_frame(size_t *out_len);
int stub_target_slave_spi_rearm(uint8_t *buf, size_t max_size);
int stub_target_slave_spi_tx_frame(const void *data, size_t len);

#ifdef __cplusplus
}
#endif
