/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * ESP32-S3 SDMMC host driver — polled IDMAC, single 512-byte descriptor per
 * transfer.  Sufficient for sector-grain reads/writes to an SD card or eMMC
 * in 1/4/8-bit mode.  Programming model follows IDF v6.0 sdmmc_host.c.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/rom_wrappers.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/sdmmc.h>

#include <soc/gpio_sig_map.h>
#include <soc/io_mux_reg.h>
#include <soc/reg_base.h>
#include <soc/sdmmc_reg.h>
#include <soc/system_reg.h>

// ROM functions
extern void esp_rom_gpio_pad_select_gpio(uint32_t gpio_num);
extern void esp_rom_gpio_connect_out_signal(uint32_t gpio_num, uint32_t signal_idx, bool out_inv, bool oen_inv);
extern void esp_rom_gpio_connect_in_signal(uint32_t gpio_num, uint32_t signal_idx, bool inv);
extern void gpio_output_enable(uint32_t gpio_num);
extern void gpio_pad_set_drv(uint32_t gpio_num, uint32_t drv);
extern void gpio_pad_input_enable(uint32_t gpio_num);

// Clock source: XTAL (40 MHz), not PLL_F160M.  ROM bootloader does not enable
// PLL_F160M (IDF's bootloader does, which is why IDF's esp_clk_tree_enable_src
// is a no-op on S3) — selecting clk_sel=1 in stub mode leaves the CIU without
// a clock and update_clk_reg never completes.  XTAL caps card clock at 20 MHz,
// matching SD Default Speed.
#define HOST_DIV_FACTOR_L    1U // div=2 → 40/2 = 20 MHz host
#define HOST_DIV_FACTOR_N    1U
#define HOST_DIV_FACTOR_H    0U
#define HOST_CLOCK_HZ        20000000U
#define INIT_FREQ_KHZ        400U // ID-mode upper bound
#define DEFAULT_FREQ_KHZ     20000U

#define CMD_DONE_TIMEOUT_US  1000000U
#define DATA_DONE_TIMEOUT_US 3000000U
#define CARD_BUSY_TIMEOUT_US 3000000U
#define ACMD41_TIMEOUT_US    1000000U

// OCR voltage window 2.7-3.6 V; HCS bit (30) requests SDHC.
#define ACMD41_OCR_ARG       0x40FF8000U
#define CMD8_ARG             0x000001AAU
#define CMD8_PATTERN         0xAAU

// IDMAC descriptor (DWC_mshc).  Plain u32 fields rather than bit-fields to
// keep the volatile DMA-visible layout under our control.
typedef struct {
    volatile uint32_t ctrl;
    volatile uint32_t sizes;
    volatile uint32_t buf1_ptr;
    volatile uint32_t buf2_or_next;
} sdmmc_desc_t;

#define DESC_CTRL_DISABLE_INT_ON_COMP BIT(1)
#define DESC_CTRL_LAST                BIT(2)
#define DESC_CTRL_FIRST               BIT(3)
#define DESC_CTRL_END_OF_RING         BIT(5)
#define DESC_CTRL_OWNED_BY_IDMAC      BIT(31)
#define DESC_SIZES_BUF1_M             0x1FFFU

// ---- BSS state ---------------------------------------------------------- //

static stub_target_sdmmc_card_info_t s_card;
static struct {
    uint8_t slot;
    uint8_t width;
    bool initialized;
} s_host;

// 16-byte aligned per DWC_mshc IDMAC requirements.
static sdmmc_desc_t s_desc __attribute__((aligned(16)));

// Captured continuously so plugin error handlers can surface "which stage died
// with what RINTSTS" instead of just a generic timeout code.
static struct {
    uint8_t stage; // SDMMC_DIAG_STAGE_* or SD/MMC CMD index 0..63
    uint32_t rintsts;
} s_diag;

static inline void diag_set_stage(uint8_t stage)
{
    s_diag.stage = stage;
}
static inline void diag_capture_rintsts(void)
{
    s_diag.rintsts = REG_READ(SDMMC_RINTSTS_REG);
}

// ---- Low-level helpers -------------------------------------------------- //

static int wait_cmd_taken(void)
{
    // CIU accepts the command when start_command self-clears.
    int timeout = (int)(CMD_DONE_TIMEOUT_US / 10U);
    while (REG_READ(SDMMC_CMD_REG) & SDMMC_CMD_START_CMD) {
        if (--timeout <= 0) {
            diag_capture_rintsts();
            return SDMMC_ERR_TIMEOUT;
        }
        stub_lib_delay_us(10);
    }
    return 0;
}

// Synchronise clkdiv/clksrc/clkena into the CIU clock domain.  USE_HOLD_REG is
// mandatory: IDF sets it on every command (including this pseudo-command).
static int update_clk_reg(uint8_t slot)
{
    diag_set_stage(SDMMC_DIAG_STAGE_UPDATE_CLK);
    int err = wait_cmd_taken();
    if (err)
        return err;

    uint32_t cmd = SDMMC_CMD_UPDATE_CLK_REG | SDMMC_CMD_WAIT_PRVDATA | SDMMC_CMD_USE_HOLD_REG | SDMMC_CMD_START_CMD |
                   (((uint32_t)slot & 0x1FU) << 16);
    REG_WRITE(SDMMC_CMDARG_REG, 0);
    REG_WRITE(SDMMC_CMD_REG, cmd);

    err = wait_cmd_taken();
    if (err)
        return err;

    // HLE (Hardware Locked Error) means the CIU was busy; retry once.
    if (REG_READ(SDMMC_RINTSTS_REG) & SDMMC_INT_HLE) {
        REG_WRITE(SDMMC_RINTSTS_REG, SDMMC_INT_HLE);
        REG_WRITE(SDMMC_CMD_REG, cmd);
        err = wait_cmd_taken();
        if (err)
            return err;
    }
    return 0;
}

static int set_card_clock(uint8_t slot, uint32_t freq_khz)
{
    // card_clk = HOST_CLOCK_HZ / (2 * div); div == 0 = bypass.
    uint32_t div;
    if (freq_khz == 0 || (freq_khz * 1000U) >= HOST_CLOCK_HZ) {
        div = 0;
    } else {
        div = (HOST_CLOCK_HZ / 1000U) / (2U * freq_khz);
        if (div == 0)
            div = 1;
        if (div > 0xFF)
            div = 0xFF;
    }

    // Apply divider + enable cclk together, then issue a single sync command.
    // The standard "disable cclk, sync, set div, sync, enable cclk, sync"
    // sequence wedges on the first sync when the controller has no card clock
    // yet — giving it the divider up front avoids that.
    uint32_t clkdiv = REG_READ(SDMMC_CLKDIV_REG);
    uint32_t clksrc = REG_READ(SDMMC_CLKSRC_REG);
    uint32_t shift = slot * 8U;
    clkdiv = (clkdiv & ~(0xFFU << shift)) | ((div & 0xFFU) << shift);
    clksrc = (clksrc & ~(0x3U << (slot * 2U))) | ((uint32_t)slot << (slot * 2U));
    REG_WRITE(SDMMC_CLKDIV_REG, clkdiv);
    REG_WRITE(SDMMC_CLKSRC_REG, clksrc);

    uint32_t clkena = REG_READ(SDMMC_CLKENA_REG);
    clkena |= (1U << slot);
    clkena &= ~(uint32_t)(1U << (slot + 16)); // clear low-power gate
    REG_WRITE(SDMMC_CLKENA_REG, clkena);

    return update_clk_reg(slot);
}

// ---- GPIO routing ------------------------------------------------------- //

static int route_pin_out(uint32_t pin, uint32_t signal, bool also_input)
{
    if (pin >= MAX_GPIO_NUM)
        return SDMMC_ERR_PIN_INVALID;
    esp_rom_gpio_pad_select_gpio(pin);
    gpio_pad_input_enable(pin);
    gpio_pad_set_drv(pin, 3); // highest drive — SD lines need fast edges
    esp_rom_gpio_connect_out_signal(pin, signal, false, false);
    if (also_input) {
        esp_rom_gpio_connect_in_signal(pin, signal, false);
    }
    gpio_output_enable(pin);
    return 0;
}

static int route_pins(uint8_t slot, const stub_target_sdmmc_attach_config_t *cfg)
{
    const uint32_t cclk_sig = (slot == 0) ? SDHOST_CCLK_OUT_1_IDX : SDHOST_CCLK_OUT_2_IDX;
    const uint32_t ccmd_sig = (slot == 0) ? SDHOST_CCMD_1_IDX : SDHOST_CCMD_2_IDX;
    const uint32_t cdata_sig_base = (slot == 0) ? SDHOST_CDATA0_1_IDX : SDHOST_CDATA0_2_IDX;
    const uint32_t cd_sig = (slot == 0) ? SDHOST_CARD_DETECT_N_1_IDX : SDHOST_CARD_DETECT_N_2_IDX;
    const uint32_t wp_sig = (slot == 0) ? SDHOST_CARD_WRITE_PRT_1_IDX : SDHOST_CARD_WRITE_PRT_2_IDX;
    const uint32_t cint_sig = (slot == 0) ? SDHOST_CARD_INT_N_1_IDX : SDHOST_CARD_INT_N_2_IDX;

    int err = route_pin_out(cfg->pin_clk, cclk_sig, false);
    if (err)
        return err;
    err = route_pin_out(cfg->pin_cmd, ccmd_sig, true);
    if (err)
        return err;
    for (uint32_t i = 0; i < cfg->width; i++) {
        err = route_pin_out(cfg->pin_d[i], cdata_sig_base + i, true);
        if (err)
            return err;
    }

    // Tie status signals to safe constants so the data FSM sees defined inputs.
    // Without these the controller waits for card_detect to assert and parts of
    // the init sequence wedge.  Matches IDF's sdmmc_slot_io_config.
    //   card_int      = high  (no interrupt — harmless for SD card)
    //   card_detect   = low   ("card present" — active-low)
    //   write_protect = high  ("not protected" — active-low)
    esp_rom_gpio_connect_in_signal(SDMMC_GPIO_MATRIX_CONST_ONE_INPUT, cint_sig, false);
    esp_rom_gpio_connect_in_signal(SDMMC_GPIO_MATRIX_CONST_ZERO_INPUT, cd_sig, false);
    esp_rom_gpio_connect_in_signal(SDMMC_GPIO_MATRIX_CONST_ONE_INPUT, wp_sig, false);
    return 0;
}

// ---- Host init ---------------------------------------------------------- //

static void host_clock_init(void)
{
    // clk_sel=0 → XTAL.  90° delay on data-out for SDR12/25 timing margin.
    uint32_t clk = (HOST_DIV_FACTOR_L & SDMMC_CLK_DIV_FACTOR_L_M) << SDMMC_CLK_DIV_FACTOR_L_S |
                   (HOST_DIV_FACTOR_N & SDMMC_CLK_DIV_FACTOR_N_M) << SDMMC_CLK_DIV_FACTOR_N_S |
                   (HOST_DIV_FACTOR_H & SDMMC_CLK_DIV_FACTOR_H_M) << SDMMC_CLK_DIV_FACTOR_H_S |
                   (1U & SDMMC_CLK_PHASE_DOUT_M) << SDMMC_CLK_PHASE_DOUT_S;
    REG_WRITE(SDMMC_CLOCK_REG, clk);
}

static int host_reset_and_dma_init(uint8_t slot)
{
    REG_WRITE(SDMMC_CTRL_REG, SDMMC_CTRL_CONTROLLER_RESET | SDMMC_CTRL_DMA_RESET | SDMMC_CTRL_FIFO_RESET);
    int timeout = 5000;
    while (REG_READ(SDMMC_CTRL_REG) & SDMMC_CTRL_CONTROLLER_RESET) {
        if (--timeout <= 0) {
            diag_capture_rintsts();
            return SDMMC_ERR_TIMEOUT;
        }
        stub_lib_delay_us(10);
    }

    // Clear interrupts and cycle int_enable off→on (matches IDF; needed for the
    // CIU to advance state machines reliably in our polled mode).
    REG_WRITE(SDMMC_RINTSTS_REG, 0xFFFFFFFFU);
    REG_WRITE(SDMMC_INTMASK_REG, 0);
    REG_WRITE(SDMMC_CTRL_REG, REG_READ(SDMMC_CTRL_REG) & ~SDMMC_CTRL_INT_ENABLE);
    REG_WRITE(SDMMC_CTRL_REG, REG_READ(SDMMC_CTRL_REG) | SDMMC_CTRL_INT_ENABLE);

    // IDMAC live for the duration; FIFO is reset before each transfer.
    REG_WRITE(SDMMC_CTRL_REG, REG_READ(SDMMC_CTRL_REG) | SDMMC_CTRL_DMA_ENABLE | SDMMC_CTRL_USE_INTERNAL_DMA);
    REG_WRITE(SDMMC_BMOD_REG, 0);
    REG_WRITE(SDMMC_BMOD_REG, SDMMC_BMOD_SW_RESET);
    REG_WRITE(SDMMC_BMOD_REG, SDMMC_BMOD_ENABLE | SDMMC_BMOD_FIXED_BURST);

    REG_WRITE(SDMMC_BLKSIZ_REG, SDMMC_BLOCK_SIZE);
    REG_WRITE(SDMMC_TMOUT_REG, SDMMC_TMOUT_DEFAULT);
    REG_WRITE(SDMMC_PWREN_REG, REG_READ(SDMMC_PWREN_REG) | (1U << slot));
    return 0;
}

// ---- Command issue + response ------------------------------------------- //

typedef enum {
    RESP_NONE = 0,
    RESP_R1,  // 48-bit, CRC checked
    RESP_R1B, // R1 + busy on data line
    RESP_R2,  // 136-bit (CID/CSD), CRC checked
    RESP_R3,  // 48-bit, no CRC (OCR)
    RESP_R6,  // 48-bit, CRC checked (RCA)
    RESP_R7,  // 48-bit, CRC checked (IF cond)
} resp_type_t;

// Branches not a switch — GCC's CSWTCH jump table would land in .rodata, and
// the plugin link rejects any non-empty .rodata.
static uint32_t cmd_flags_for_response(resp_type_t r)
{
    uint32_t flags = 0;
    if (r != RESP_NONE) {
        flags |= SDMMC_CMD_RESP_EXPECT;
        if (r != RESP_R3)
            flags |= SDMMC_CMD_RESP_CRC; // R3 (OCR) has no CRC
        if (r == RESP_R2)
            flags |= SDMMC_CMD_RESP_LONG;
    }
    return flags;
}

static int wait_cmd_done(uint32_t *rintsts_out)
{
    int timeout = (int)(CMD_DONE_TIMEOUT_US / 10U);
    while (1) {
        uint32_t st = REG_READ(SDMMC_RINTSTS_REG);
        if (st & SDMMC_INT_CMD_DONE) {
            s_diag.rintsts = st;
            if (rintsts_out)
                *rintsts_out = st;
            REG_WRITE(SDMMC_RINTSTS_REG, st & (SDMMC_INT_CMD_DONE | SDMMC_INT_CMD_ERR));
            return 0;
        }
        if (--timeout <= 0) {
            s_diag.rintsts = st;
            return SDMMC_ERR_TIMEOUT;
        }
        stub_lib_delay_us(10);
    }
}

static int wait_data_done(uint32_t *rintsts_out)
{
    int timeout = (int)(DATA_DONE_TIMEOUT_US / 10U);
    while (1) {
        uint32_t st = REG_READ(SDMMC_RINTSTS_REG);
        if (st & SDMMC_INT_DTO) {
            s_diag.rintsts = st;
            if (rintsts_out)
                *rintsts_out = st;
            REG_WRITE(SDMMC_RINTSTS_REG, st & (SDMMC_INT_DTO | SDMMC_INT_RXDR | SDMMC_INT_TXDR | SDMMC_INT_DATA_ERR));
            return 0;
        }
        if (--timeout <= 0) {
            s_diag.rintsts = st;
            return SDMMC_ERR_TIMEOUT;
        }
        stub_lib_delay_us(10);
    }
}

static int wait_card_idle(void)
{
    diag_set_stage(SDMMC_DIAG_STAGE_WAIT_BUSY);
    int timeout = (int)(CARD_BUSY_TIMEOUT_US / 10U);
    while (REG_READ(SDMMC_STATUS_REG) & SDMMC_STATUS_DATA_BUSY) {
        if (--timeout <= 0) {
            diag_capture_rintsts();
            return SDMMC_ERR_TIMEOUT;
        }
        stub_lib_delay_us(10);
    }
    return 0;
}

static int send_cmd(uint8_t idx, uint32_t arg, resp_type_t r, bool send_init, uint32_t *resp_out)
{
    diag_set_stage(idx);
    int err = wait_cmd_taken();
    if (err)
        return err;

    REG_WRITE(SDMMC_RINTSTS_REG, SDMMC_INT_CMD_DONE | SDMMC_INT_CMD_ERR);

    uint32_t cmd = (uint32_t)idx & SDMMC_CMD_INDEX_M;
    cmd |= cmd_flags_for_response(r);
    cmd |= SDMMC_CMD_USE_HOLD_REG | SDMMC_CMD_WAIT_PRVDATA | SDMMC_CMD_START_CMD;
    cmd |= ((uint32_t)s_host.slot & 0x1FU) << 16;
    if (send_init)
        cmd |= SDMMC_CMD_SEND_INIT;

    REG_WRITE(SDMMC_CMDARG_REG, arg);
    REG_WRITE(SDMMC_CMD_REG, cmd);

    uint32_t rintsts = 0;
    err = wait_cmd_done(&rintsts);
    if (err)
        return err;
    if (rintsts & SDMMC_INT_CMD_ERR) {
        // RTO is a normal "no card" / "ACMD41 still busy" signal — caller decodes.
        return (rintsts & SDMMC_INT_RTO) ? SDMMC_ERR_TIMEOUT : SDMMC_ERR_CMD_FAIL;
    }

    if (resp_out) {
        resp_out[0] = REG_READ(SDMMC_RESP0_REG);
        if (r == RESP_R2) {
            resp_out[1] = REG_READ(SDMMC_RESP1_REG);
            resp_out[2] = REG_READ(SDMMC_RESP2_REG);
            resp_out[3] = REG_READ(SDMMC_RESP3_REG);
        }
    }

    if (r == RESP_R1B)
        return wait_card_idle();
    return 0;
}

// ---- Single-block IDMAC transfer ---------------------------------------- //

static int xfer_single_block(uint8_t cmd_idx, uint32_t arg, void *buf, bool is_write)
{
    diag_set_stage(SDMMC_DIAG_STAGE_DATA_XFER);
    if ((((uintptr_t)buf) & 0x3U) != 0)
        return SDMMC_ERR_BAD_ARG;

    REG_WRITE(SDMMC_CTRL_REG, REG_READ(SDMMC_CTRL_REG) | SDMMC_CTRL_FIFO_RESET);
    int timeout = 100;
    while (REG_READ(SDMMC_CTRL_REG) & SDMMC_CTRL_FIFO_RESET) {
        if (--timeout <= 0)
            return SDMMC_ERR_TIMEOUT;
        stub_lib_delay_us(1);
    }

    s_desc.sizes = (SDMMC_BLOCK_SIZE & DESC_SIZES_BUF1_M);
    s_desc.buf1_ptr = (uint32_t)(uintptr_t)buf;
    s_desc.buf2_or_next = 0;
    s_desc.ctrl = DESC_CTRL_OWNED_BY_IDMAC | DESC_CTRL_FIRST | DESC_CTRL_LAST | DESC_CTRL_END_OF_RING |
                  DESC_CTRL_DISABLE_INT_ON_COMP;

    REG_WRITE(SDMMC_BYTCNT_REG, SDMMC_BLOCK_SIZE);
    REG_WRITE(SDMMC_BLKSIZ_REG, SDMMC_BLOCK_SIZE);
    REG_WRITE(SDMMC_DBADDR_REG, (uint32_t)(uintptr_t)&s_desc);

    REG_WRITE(SDMMC_RINTSTS_REG,
              SDMMC_INT_DTO | SDMMC_INT_CMD_DONE | SDMMC_INT_CMD_ERR | SDMMC_INT_DATA_ERR | SDMMC_INT_RXDR |
                  SDMMC_INT_TXDR);

    int err = wait_cmd_taken();
    if (err)
        return err;

    uint32_t cmd = (uint32_t)cmd_idx & SDMMC_CMD_INDEX_M;
    cmd |= cmd_flags_for_response(RESP_R1) | SDMMC_CMD_DATA_EXPECTED;
    cmd |= SDMMC_CMD_USE_HOLD_REG | SDMMC_CMD_WAIT_PRVDATA | SDMMC_CMD_START_CMD;
    cmd |= ((uint32_t)s_host.slot & 0x1FU) << 16;
    if (is_write)
        cmd |= SDMMC_CMD_WRITE;

    REG_WRITE(SDMMC_CMDARG_REG, arg);
    REG_WRITE(SDMMC_CMD_REG, cmd);

    uint32_t rintsts = 0;
    err = wait_cmd_done(&rintsts);
    if (err)
        return err;
    if (rintsts & SDMMC_INT_CMD_ERR)
        return SDMMC_ERR_CMD_FAIL;

    err = wait_data_done(&rintsts);
    if (err)
        return err;
    if (rintsts & SDMMC_INT_DATA_ERR)
        return SDMMC_ERR_DATA_FAIL;

    if (is_write)
        return wait_card_idle();
    return 0;
}

// ---- Card init sequence ------------------------------------------------- //

static int card_init(uint8_t target_width)
{
    // CMD0 — go idle, with 80 init clocks for power-on.
    int err = send_cmd(0, 0, RESP_NONE, true, NULL);
    if (err)
        return err;
    stub_lib_delay_us(2000);

    // CMD8 — distinguish SD 2.0 from SD 1.x / MMC.  RTO here is normal for
    // pre-2.0 SD or MMC; we proceed with HCS cleared in that case.
    uint32_t resp[4] = { 0 };
    bool sd_v2 = false;
    err = send_cmd(8, CMD8_ARG, RESP_R7, false, resp);
    if (err == 0 && (resp[0] & 0xFFU) == CMD8_PATTERN)
        sd_v2 = true;

    // ACMD41 — SD operating condition.  Loop until OCR busy bit (31) sets.
    bool got_sd = false;
    uint32_t ocr_arg = sd_v2 ? ACMD41_OCR_ARG : (ACMD41_OCR_ARG & ~BIT(30));
    int timeout = (int)(ACMD41_TIMEOUT_US / 1000U);
    while (timeout-- > 0) {
        err = send_cmd(55, 0, RESP_R1, false, NULL);
        if (err)
            break;
        err = send_cmd(41, ocr_arg, RESP_R3, false, resp);
        if (err)
            break;
        if (resp[0] & BIT(31)) {
            s_card.ocr = resp[0];
            s_card.is_high_capacity = (resp[0] & BIT(30)) ? 1 : 0;
            got_sd = true;
            break;
        }
        stub_lib_delay_us(1000);
    }

    // Fall back to eMMC via CMD1 if SD init didn't latch.
    if (!got_sd) {
        timeout = (int)(ACMD41_TIMEOUT_US / 1000U);
        while (timeout-- > 0) {
            err = send_cmd(1, 0x40FF8000U, RESP_R3, false, resp);
            if (err)
                continue;
            if (resp[0] & BIT(31)) {
                s_card.ocr = resp[0];
                s_card.is_high_capacity = (resp[0] & BIT(30)) ? 1 : 0;
                s_card.is_mmc = 1;
                break;
            }
            stub_lib_delay_us(1000);
        }
        if (!(resp[0] & BIT(31)))
            return SDMMC_ERR_NO_CARD;
    }

    // CMD2 — fetch CID (R2).
    err = send_cmd(2, 0, RESP_R2, false, s_card.cid);
    if (err)
        return err;

    // CMD3 — get RCA (SD) / set RCA (MMC).
    if (s_card.is_mmc) {
        err = send_cmd(3, 0x00010000U, RESP_R1, false, resp);
        if (err)
            return err;
        s_card.rca = 0x0001U;
    } else {
        err = send_cmd(3, 0, RESP_R6, false, resp);
        if (err)
            return err;
        s_card.rca = (resp[0] >> 16) & 0xFFFFU;
    }

    // CMD9 — get CSD (R2).
    err = send_cmd(9, (uint32_t)s_card.rca << 16, RESP_R2, false, s_card.csd);
    if (err)
        return err;

    // CSD v2 (SDHC/SDXC): capacity = (C_SIZE + 1) * 512 KiB.
    // CSD v1 (SDSC):     capacity = (C_SIZE + 1) * 2^(C_SIZE_MULT+2) * 2^READ_BL_LEN.
    uint32_t csd_struct = (s_card.csd[3] >> 30) & 0x3U;
    if (csd_struct == 1U) {
        uint64_t c_size = (((uint64_t)s_card.csd[2] & 0x3FU) << 16) | (s_card.csd[1] >> 16);
        s_card.capacity_bytes = (c_size + 1ULL) * 512ULL * 1024ULL;
    } else {
        uint32_t read_bl_len = (s_card.csd[2] >> 16) & 0xFU;
        uint32_t c_size = ((s_card.csd[2] & 0x3FFU) << 2) | (s_card.csd[1] >> 30);
        uint32_t c_size_mult = (s_card.csd[1] >> 15) & 0x7U;
        uint64_t blocknr = ((uint64_t)c_size + 1ULL) << (c_size_mult + 2U);
        s_card.capacity_bytes = blocknr << read_bl_len;
    }

    // CMD7 — select card, enter TRAN.
    err = send_cmd(7, (uint32_t)s_card.rca << 16, RESP_R1B, false, resp);
    if (err)
        return err;

    // Bus width: ACMD6 for SD; eMMC CMD6 SWITCH not implemented.
    if (!s_card.is_mmc && target_width == 4U) {
        err = send_cmd(55, (uint32_t)s_card.rca << 16, RESP_R1, false, NULL);
        if (err)
            return err;
        err = send_cmd(6, 0x2U, RESP_R1, false, NULL);
        if (err)
            return err;
        REG_WRITE(SDMMC_CTYPE_REG, (REG_READ(SDMMC_CTYPE_REG) & ~(1U << (s_host.slot + 16))) | (1U << s_host.slot));
        s_card.width = 4;
    } else if (target_width == 8U) {
        return SDMMC_ERR_UNSUPPORTED;
    } else {
        REG_WRITE(SDMMC_CTYPE_REG, REG_READ(SDMMC_CTYPE_REG) & ~((1U << s_host.slot) | (1U << (s_host.slot + 16))));
        s_card.width = 1;
    }

    // CMD16 — fix block length at 512 (required by eMMC/SDSC; SDHC ignores).
    return send_cmd(16, SDMMC_BLOCK_SIZE, RESP_R1, false, NULL);
}

// ---- Public API --------------------------------------------------------- //

// Defaults match ESP-IDF v6.0 SDMMC_SLOT_CONFIG_DEFAULT for ESP32-S3.  Inlined
// rather than a const table to keep the plugin .rodata-free.
static void apply_default_cfg(stub_target_sdmmc_attach_config_t *out)
{
    out->slot = 0;
    out->width = 4;
    out->freq_khz = DEFAULT_FREQ_KHZ;
    out->cd_pin = 0xFFU;
    out->wp_pin = 0xFFU;
    out->pin_clk = 14U;
    out->pin_cmd = 15U;
    out->pin_d[0] = 2U;
    out->pin_d[1] = 4U;
    out->pin_d[2] = 12U;
    out->pin_d[3] = 13U;
    out->pin_d[4] = 33U;
    out->pin_d[5] = 34U;
    out->pin_d[6] = 35U;
    out->pin_d[7] = 36U;
}

int stub_target_sdmmc_attach(const stub_target_sdmmc_attach_config_t *cfg)
{
    stub_target_sdmmc_attach_config_t local;
    if (cfg == NULL || cfg->slot == 0xFFU) {
        apply_default_cfg(&local);
    } else {
        local = *cfg;
        if (local.width != 1 && local.width != 4 && local.width != 8)
            return SDMMC_ERR_BAD_ARG;
        if (local.freq_khz == 0)
            local.freq_khz = DEFAULT_FREQ_KHZ;
    }
    if (local.slot > 1)
        return SDMMC_ERR_BAD_ARG;

    memset(&s_card, 0, sizeof(s_card));
    s_diag.stage = SDMMC_DIAG_STAGE_NONE;
    s_diag.rintsts = 0;
    s_host.slot = local.slot;
    s_host.width = local.width;
    s_host.initialized = false;

    // Bring up peripheral clock and pulse module reset.
    REG_SET_BIT(SYSTEM_PERIP_CLK_EN1_REG, SYSTEM_SDIO_HOST_CLK_EN);
    REG_SET_BIT(SYSTEM_PERIP_RST_EN1_REG, SYSTEM_SDIO_HOST_RST);
    REG_CLR_BIT(SYSTEM_PERIP_RST_EN1_REG, SYSTEM_SDIO_HOST_RST);
    stub_lib_delay_us(1000);

    // Pins (with const card_detect) routed before CTRL reset so the data FSM
    // sees defined inputs as it comes out of reset.
    int err = route_pins(local.slot, &local);
    if (err)
        return err;

    host_clock_init();
    err = host_reset_and_dma_init(local.slot);
    if (err)
        return err;

    err = set_card_clock(local.slot, INIT_FREQ_KHZ); // <= 400 kHz for ID mode
    if (err)
        return err;
    stub_lib_delay_us(1000);

    err = card_init(local.width);
    if (err)
        return err;

    err = set_card_clock(local.slot, local.freq_khz);
    if (err)
        return err;

    s_host.initialized = true;
    return 0;
}

const stub_target_sdmmc_card_info_t *stub_target_sdmmc_get_card_info(void)
{
    return s_host.initialized ? &s_card : NULL;
}

int stub_target_sdmmc_read_sector(uint32_t lba, uint8_t *buf)
{
    if (!s_host.initialized)
        return SDMMC_ERR_NOT_INITIALIZED;
    uint32_t arg = s_card.is_high_capacity ? lba : (lba * SDMMC_BLOCK_SIZE);
    return xfer_single_block(17, arg, buf, false);
}

int stub_target_sdmmc_write_sector(uint32_t lba, const uint8_t *buf)
{
    if (!s_host.initialized)
        return SDMMC_ERR_NOT_INITIALIZED;
    uint32_t arg = s_card.is_high_capacity ? lba : (lba * SDMMC_BLOCK_SIZE);
    // xfer_single_block writes don't mutate the buffer; cast is safe.
    return xfer_single_block(24, arg, (void *)(uintptr_t)buf, true);
}

int stub_target_sdmmc_erase_range(uint32_t start_lba, uint32_t end_lba)
{
    if (!s_host.initialized)
        return SDMMC_ERR_NOT_INITIALIZED;
    if (end_lba < start_lba)
        return SDMMC_ERR_BAD_ARG;

    uint32_t start_arg = s_card.is_high_capacity ? start_lba : (start_lba * SDMMC_BLOCK_SIZE);
    uint32_t end_arg = s_card.is_high_capacity ? end_lba : (end_lba * SDMMC_BLOCK_SIZE);

    int err = send_cmd(s_card.is_mmc ? 35 : 32, start_arg, RESP_R1, false, NULL);
    if (err)
        return err;
    err = send_cmd(s_card.is_mmc ? 36 : 33, end_arg, RESP_R1, false, NULL);
    if (err)
        return err;
    return send_cmd(38, 0, RESP_R1B, false, NULL);
}

void stub_target_sdmmc_get_last_diag(uint8_t *cmd_idx_out, uint32_t *rintsts_out)
{
    if (cmd_idx_out)
        *cmd_idx_out = s_diag.stage;
    if (rintsts_out)
        *rintsts_out = s_diag.rintsts;
}
