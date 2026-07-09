/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/err.h>
#include <esp-stub-lib/log.h>
#include <esp-stub-lib/spi.h>

#include <target/spi.h>

#define SPI_DMA_ALIGN 4U

bool stub_lib_spi_is_active(void)
{
    return stub_target_spi_is_active();
}

void stub_lib_spi_init(void)
{
    stub_target_spi_init();
}

bool stub_lib_spi_take_rx_frame(size_t *out_len)
{
    return stub_target_spi_take_rx_frame(out_len);
}

int stub_lib_spi_rearm(uint8_t *buf, size_t max_size)
{
    if (buf == NULL || max_size == 0 || !IS_ALIGNED((uintptr_t)buf, SPI_DMA_ALIGN)) {
        STUB_LOGE("Invalid SPI RX buffer\n");
        return STUB_LIB_ERR_INVALID_ARG;
    }
    int ret = stub_target_spi_rearm(buf, max_size);
    if (ret != STUB_LIB_OK) {
        STUB_LOGE("Failed to arm SPI RX\n");
        return ret;
    }
    return STUB_LIB_OK;
}

int stub_lib_spi_tx_frame(const void *data, size_t len)
{
    if (data == NULL || len == 0 || len > SPI_DMA_DESC_MAX_LEN || !IS_ALIGNED((uintptr_t)data, SPI_DMA_ALIGN)) {
        STUB_LOGE("Invalid SPI TX frame\n");
        return STUB_LIB_ERR_INVALID_ARG;
    }
    int ret = stub_target_spi_tx_frame(data, len);
    if (ret != STUB_LIB_OK) {
        STUB_LOGE("Failed to send SPI frame\n");
        return ret;
    }
    return STUB_LIB_OK;
}
