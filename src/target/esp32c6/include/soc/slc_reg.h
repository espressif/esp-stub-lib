/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * SLC (Serial Link Controller) register definitions for ESP32-C6.
 * Base address: DR_REG_SLC_BASE = 0x60017000
 *
 * Naming note: SLC uses "TX" for host→slave and "RX" for slave→host,
 * which is the opposite of application-level terminology.
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>

#include <soc/reg_base.h>

/* Register addresses */
#define SDIO_SLCCONF0_REG           (DR_REG_SLC_BASE + 0x00)
#define SDIO_SLC0INT_RAW_REG        (DR_REG_SLC_BASE + 0x04)
#define SDIO_SLC0INT_ST_REG         (DR_REG_SLC_BASE + 0x08)
#define SDIO_SLC0INT_ENA_REG        (DR_REG_SLC_BASE + 0x0C)
#define SDIO_SLC0INT_CLR_REG        (DR_REG_SLC_BASE + 0x10)

/* DMA link registers */
#define SDIO_SLC0RX_LINK_REG        (DR_REG_SLC_BASE + 0x3C) /* Slave→host DMA control */
#define SDIO_SLC0RX_LINK_ADDR_REG   (DR_REG_SLC_BASE + 0x40) /* Slave→host first descriptor */
#define SDIO_SLC0TX_LINK_REG        (DR_REG_SLC_BASE + 0x44) /* Host→slave DMA control */
#define SDIO_SLC0TX_LINK_ADDR_REG   (DR_REG_SLC_BASE + 0x48) /* Host→slave first descriptor */
#define SDIO_SLC0TOKEN1_REG         (DR_REG_SLC_BASE + 0x64) /* Host→slave buffer credits */
#define SDIO_SLC_RX_DSCR_CONF_REG   (DR_REG_SLC_BASE + 0xA8) /* Slave→host descriptor configuration */

/* SDIO_SLC0TOKEN1_REG fields */
#define SDIO_SLC0_TOKEN1_WDATA_MASK 0x0FFFU
#define SDIO_SLC0_TOKEN1_WR         BIT(12)

/* SDIO_SLC_RX_DSCR_CONF_REG fields */
#define SDIO_SLC0_TOKEN_NO_REPLACE  BIT(0)

/* SDIO_SLC0RX_LINK_REG control bits (slave→host DMA) */
#define SDIO_SLC0_RXLINK_STOP       BIT(28)
#define SDIO_SLC0_RXLINK_START      BIT(29)

/* SDIO_SLC0TX_LINK_REG control bits (host→slave DMA) */
#define SDIO_SLC0_TXLINK_STOP       BIT(28)
#define SDIO_SLC0_TXLINK_START      BIT(29)
#define SDIO_SLC0_TXLINK_RESTART    BIT(30)
#define SDIO_SLC0_TXLINK_PARK       BIT(31)

/* SDIO_SLCCONF0_REG bits — DMA reset */
#define SDIO_SLC0_TX_RST            BIT(0) /* Reset host→slave DMA */
#define SDIO_SLC0_RX_RST            BIT(1) /* Reset slave→host DMA */

/*
 * Interrupt bits — same bit position in RAW / ST / ENA / CLR registers.
 *
 * Bit 15: TX_SUC_EOF     — host finished writing a packet (host→slave data ready)
 * Bit 16: RX_DONE        — slave→host DMA descriptor processed (owner reclaimed)
 * Bit 17: RX_EOF         — slave→host DMA transfer completed (eof descriptor sent)
 * Bit 18: RX_DSCR_EMPTY  — slave→host descriptor chain exhausted; must be cleared
 *                           before restarting RXLINK_START or the new DMA is ignored
 * Bit 19: TX_DSCR_ERR    — TX descriptor error (fatal)
 * Bit 20: RX_DSCR_ERR    — RX descriptor error (fatal)
 * Bit 21: TX_DSCR_EMPTY  — TX descriptor chain exhausted (fatal)
 */
#define SDIO_SLC0_TX_SUC_EOF_INT    BIT(15)
#define SDIO_SLC0_RX_DONE_INT       BIT(16)
#define SDIO_SLC0_RX_EOF_INT        BIT(17)
#define SDIO_SLC0_RX_DSCR_EMPTY_INT BIT(18)
#define SDIO_SLC0_TX_DSCR_ERR_INT   BIT(19)
#define SDIO_SLC0_RX_DSCR_ERR_INT   BIT(20)
#define SDIO_SLC0_TX_DSCR_EMPTY_INT BIT(21)
