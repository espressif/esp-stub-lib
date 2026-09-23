/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * Shared-register SPI slave download-boot protocol constants.
 *
 * These values are defined by the ESP mask ROM (see esp-rom spi_rom_reg.h) and
 * must stay in lockstep with the host-side driver in
 * esp-serial-flasher/src/protocol_spi.c. All SPI transport targets use the
 * same W0..W3 handshake layout (VER / RXSTA / TXSTA / CMD).
 */
#pragma once

#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>

#ifdef __cplusplus
extern "C" {
#endif

/* RXSTA / TXSTA status word layout */
#define SPI_SLV_STA_TOGGLE          BIT(0)
#define SPI_SLV_STA_INIT            BIT(1)
#define SPI_SLV_STA_LEN_SHIFT       2
#define SPI_SLV_STATE_INIT          (SPI_SLV_STA_TOGGLE | SPI_SLV_STA_INIT)

/* CMD register (W3) handshake bytes */
#define SPI_SLV_CMD_IDLE            0xAAU /* slave -> host: idle, no download selected yet */
#define SPI_SLV_CMD_READY           0xA5U /* host -> slave (echoed back): connected        */
#define SPI_SLV_CMD_REBOOT          0xFEU /* host -> slave: reboot                         */
#define SPI_SLV_CMD_DONE            0x55U /* slave -> host: prior SPI download completed   */

#define SPI_SLV_VERSION_VALUE       0x20200618U /* mirrors the ROM's VERSION_VALUE */

/* Bounded wait for the host to read a TX frame out over RDDMA. */
#define SPI_SLV_TX_TIMEOUT_US       1000000U

/* Bounded wait for the DMA engine to finish draining a frame to/from RAM. */
#define SPI_SLV_DMA_DONE_TIMEOUT_US 100000U

/* DMA descriptor sizing, shared by all SPI transport targets. A single
 * descriptor's size/length field is 12 bits, so it maxes out at a 4-byte-aligned
 * 4092 bytes. */
#define LLDESC_SPI_MAX_BUFFER_SIZE  (4096U - 4U)
#define LLDESC_SPI_SIZE_MASK        0xFFFU

/* RX descriptor chain sized to cover FRAME_BUFFER_SIZE (0x4107). */
#define SPI_SLV_RX_DESC_COUNT       5U
#define SPI_SLV_RX_MAX_LEN          (SPI_SLV_RX_DESC_COUNT * LLDESC_SPI_MAX_BUFFER_SIZE)

#ifdef __cplusplus
}
#endif
