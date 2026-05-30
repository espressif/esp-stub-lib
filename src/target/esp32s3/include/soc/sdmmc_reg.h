/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * ESP32-S3 SDMMC host controller (Synopsys DWC_mshc) register definitions.
 * Bit positions follow IDF v6.0 sdmmc_struct.h.
 */
#pragma once

#include <esp-stub-lib/bit_utils.h>
#include <soc/reg_base.h>

// ---- Register offsets ----------------------------------------------------- //

#define SDMMC_CTRL_REG       (DR_REG_SDMMC_BASE + 0x000)
#define SDMMC_PWREN_REG      (DR_REG_SDMMC_BASE + 0x004)
#define SDMMC_CLKDIV_REG     (DR_REG_SDMMC_BASE + 0x008)
#define SDMMC_CLKSRC_REG     (DR_REG_SDMMC_BASE + 0x00C)
#define SDMMC_CLKENA_REG     (DR_REG_SDMMC_BASE + 0x010)
#define SDMMC_TMOUT_REG      (DR_REG_SDMMC_BASE + 0x014)
#define SDMMC_CTYPE_REG      (DR_REG_SDMMC_BASE + 0x018)
#define SDMMC_BLKSIZ_REG     (DR_REG_SDMMC_BASE + 0x01C)
#define SDMMC_BYTCNT_REG     (DR_REG_SDMMC_BASE + 0x020)
#define SDMMC_INTMASK_REG    (DR_REG_SDMMC_BASE + 0x024)
#define SDMMC_CMDARG_REG     (DR_REG_SDMMC_BASE + 0x028)
#define SDMMC_CMD_REG        (DR_REG_SDMMC_BASE + 0x02C)
#define SDMMC_RESP0_REG      (DR_REG_SDMMC_BASE + 0x030)
#define SDMMC_RESP1_REG      (DR_REG_SDMMC_BASE + 0x034)
#define SDMMC_RESP2_REG      (DR_REG_SDMMC_BASE + 0x038)
#define SDMMC_RESP3_REG      (DR_REG_SDMMC_BASE + 0x03C)
#define SDMMC_RINTSTS_REG    (DR_REG_SDMMC_BASE + 0x044)
#define SDMMC_STATUS_REG     (DR_REG_SDMMC_BASE + 0x048)
#define SDMMC_BMOD_REG       (DR_REG_SDMMC_BASE + 0x080)
#define SDMMC_DBADDR_REG     (DR_REG_SDMMC_BASE + 0x088)
// S3-specific clock register: source select, divider, phase delays.
#define SDMMC_CLOCK_REG      (DR_REG_SDMMC_BASE + 0x800)

// ---- CTRL register bits -------------------------------------------------- //

#define SDMMC_CTRL_CONTROLLER_RESET BIT(0)
#define SDMMC_CTRL_FIFO_RESET       BIT(1)
#define SDMMC_CTRL_DMA_RESET        BIT(2)
#define SDMMC_CTRL_INT_ENABLE       BIT(4)
#define SDMMC_CTRL_DMA_ENABLE       BIT(5)
#define SDMMC_CTRL_USE_INTERNAL_DMA BIT(25)

// ---- CMD register bits --------------------------------------------------- //

#define SDMMC_CMD_INDEX_M        0x3FU
#define SDMMC_CMD_RESP_EXPECT    BIT(6)
#define SDMMC_CMD_RESP_LONG      BIT(7)
#define SDMMC_CMD_RESP_CRC       BIT(8)
#define SDMMC_CMD_DATA_EXPECTED  BIT(9)
#define SDMMC_CMD_WRITE          BIT(10)
#define SDMMC_CMD_WAIT_PRVDATA   BIT(13)
#define SDMMC_CMD_SEND_INIT      BIT(15)
#define SDMMC_CMD_UPDATE_CLK_REG BIT(21)
#define SDMMC_CMD_USE_HOLD_REG   BIT(29)
#define SDMMC_CMD_START_CMD      BIT(31)

// ---- RINTSTS bits -------------------------------------------------------- //

#define SDMMC_INT_CD       BIT(0)
#define SDMMC_INT_RESP_ERR BIT(1)
#define SDMMC_INT_CMD_DONE BIT(2)
#define SDMMC_INT_DTO      BIT(3)
#define SDMMC_INT_TXDR     BIT(4)
#define SDMMC_INT_RXDR     BIT(5)
#define SDMMC_INT_RCRC     BIT(6)
#define SDMMC_INT_DCRC     BIT(7)
#define SDMMC_INT_RTO      BIT(8)
#define SDMMC_INT_DRTO     BIT(9)
#define SDMMC_INT_HTO      BIT(10)
#define SDMMC_INT_FRUN     BIT(11)
#define SDMMC_INT_HLE      BIT(12)
#define SDMMC_INT_SBE      BIT(13)
#define SDMMC_INT_EBE      BIT(15)
#define SDMMC_INT_CMD_ERR  (SDMMC_INT_RTO | SDMMC_INT_RCRC | SDMMC_INT_RESP_ERR | SDMMC_INT_HLE)
#define SDMMC_INT_DATA_ERR (SDMMC_INT_DRTO | SDMMC_INT_DCRC | SDMMC_INT_HTO | SDMMC_INT_FRUN | SDMMC_INT_SBE | SDMMC_INT_EBE)
#define SDMMC_INT_ALL      0xFFFFU

// ---- STATUS bits --------------------------------------------------------- //

#define SDMMC_STATUS_DATA_BUSY BIT(9)

// ---- BMOD (IDMAC) bits --------------------------------------------------- //

#define SDMMC_BMOD_SW_RESET    BIT(0)
#define SDMMC_BMOD_FIXED_BURST BIT(1)
#define SDMMC_BMOD_ENABLE      BIT(7)

// ---- CLOCK_REG (0x800) fields ------------------------------------------- //

#define SDMMC_CLK_PHASE_DOUT_S   0
#define SDMMC_CLK_PHASE_DOUT_M   0x7U
#define SDMMC_CLK_DIV_FACTOR_H_S 9
#define SDMMC_CLK_DIV_FACTOR_H_M 0xFU
#define SDMMC_CLK_DIV_FACTOR_L_S 13
#define SDMMC_CLK_DIV_FACTOR_L_M 0xFU
#define SDMMC_CLK_DIV_FACTOR_N_S 17
#define SDMMC_CLK_DIV_FACTOR_N_M 0xFU
#define SDMMC_CLK_SEL            BIT(23) // 0: XTAL (40 MHz), 1: PLL_F160M

// ---- TMOUT defaults ------------------------------------------------------ //

// Generous defaults: response = 0xFF cycles, data = 0xFFFFFF cycles.
#define SDMMC_TMOUT_DEFAULT  ((0xFFFFFFU << 8) | 0xFFU)

#define SDMMC_BLOCK_SIZE 512U

// ---- GPIO matrix signal indices for SDHOST (S3) ------------------------- //
// Slot 1 (hw) → "_1_IDX"; slot 2 (hw) → "_2_IDX".  IDF API maps
// SDMMC_HOST_SLOT_0 → hw slot 1 and SDMMC_HOST_SLOT_1 → hw slot 2.
#define SDHOST_CCLK_OUT_1_IDX        172
#define SDHOST_CCLK_OUT_2_IDX        173
#define SDHOST_CCMD_1_IDX            178
#define SDHOST_CCMD_2_IDX            179
#define SDHOST_CDATA0_1_IDX          180
#define SDHOST_CDATA0_2_IDX          213
#define SDHOST_CARD_DETECT_N_1_IDX   194
#define SDHOST_CARD_DETECT_N_2_IDX   195
#define SDHOST_CARD_WRITE_PRT_1_IDX  196
#define SDHOST_CARD_WRITE_PRT_2_IDX  197
#define SDHOST_CARD_INT_N_1_IDX      198
#define SDHOST_CARD_INT_N_2_IDX      199

// GPIO matrix "constant input" sources (soc/gpio_pins.h).
#define SDMMC_GPIO_MATRIX_CONST_ONE_INPUT  0x38U
#define SDMMC_GPIO_MATRIX_CONST_ZERO_INPUT 0x3CU
