/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * SDIO transport for ESP32-C5 flasher stub.
 *
 * The ROM already completed SDIO enumeration before jumping to the stub.
 * The host→slave SLC TX DMA is NOT pre-armed for the stub (SLC0TX_LINK_ADDR
 * reads 0 at startup).  We build our own static descriptor chain of
 * SDIO_RX_NUM_DESC × SDIO_RX_BLKSZ bytes and arm the DMA with it.
 *
 * Receive path (interrupt-driven):
 *   Caller provides a receive buffer via stub_target_sdio_rearm().
 *   sdio_slc_isr() fires on TX_SUC_EOF_INT, calls slc_from_host_chain_fetch()
 *   and sdio_process_from_host(), then latches the received length for
 *   stub_target_sdio_take_rx_frame().
 *   Payload length is taken from the DMA descriptor length. No SLIP decoding.
 *
 * Transmit path (synchronous):
 *   stub_target_sdio_tx_frame() sends one aligned raw response frame with SLC RX
 *   DMA and waits until the host consumes it, so caller-owned stack buffers are
 *   safe.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <esp-stub-lib/err.h>
#include <esp-stub-lib/rom_wrappers.h>
#include <esp-stub-lib/sdio.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/sdio.h>

#include <private/helpers.h>
#include <private/rom_sdio_types.h>

#include <soc/clic_reg.h>
#include <soc/interrupt_matrix_reg.h>
#include <soc/slc_reg.h>
#include <soc/slchost_reg.h>

/* -------------------------------------------------------------------------
 * TX buffer — stub→host (raw payload, no SIP header)
 * ------------------------------------------------------------------------- */
#define SDIO_SLC_INT_CLR_ALL   0xFFFFFFFFU
#define SDIO_TX_TIMEOUT_US     1000000U
#define SDIO_SLC_INTERRUPT_NUM 18U

static lldesc_t s_tx_desc;

/* -------------------------------------------------------------------------
 * RX descriptor — host→stub (SLC TX DMA in hardware terms).
 *
 * Each DMA arm exposes a full descriptor chain over the active SLIP raw receive
 * buffer. slc_from_host_chain_fetch() returns one complete host FIFO write,
 * which can span multiple 512-byte descriptors.
 * ------------------------------------------------------------------------- */
#define SDIO_RX_BLKSZ    512U
#define SDIO_RX_NUM_DESC 33U

static lldesc_t s_rx_desc[SDIO_RX_NUM_DESC];
static volatile bool s_rx_armed;
static volatile bool s_rx_frame_ready;
static volatile size_t s_rx_received_len;

static sip_t *s_sip;

/* -------------------------------------------------------------------------
 * ROM function declarations
 * All symbols are resolved via esp32c5.rom.ld / esp32c5.rom.api.ld.
 * ------------------------------------------------------------------------- */
extern sip_t *sip_get_ptr(void);
extern bool sip_download_begin(void);
extern uint32_t _rom_eco_version;

extern int slc_from_host_chain_fetch(lldesc_t **head, lldesc_t **tail);
extern void slc_send_to_host_chain(lldesc_t *head, lldesc_t *tail);
extern void slc_to_host_chain_recycle(lldesc_t **head, lldesc_t **tail);

extern void esp_rom_isr_attach(int int_num, void *handler, void *arg);
extern void esp_rom_isr_unmask(int int_num);
extern void esp_rom_esprv_intc_int_set_priority(int int_num, int priority);

static void sdio_set_rx_credits(uint32_t credits)
{
    REG_WRITE(SDIO_SLC0TOKEN1_REG, SDIO_SLC0_TOKEN1_WR | (credits & SDIO_SLC0_TOKEN1_WDATA_MASK));
}

static void sdio_reset_from_host_window(void)
{
    REG_SET_BIT(SDIO_SLCCONF0_REG, SDIO_SLC0_TX_RST);
    REG_CLR_BIT(SDIO_SLCCONF0_REG, SDIO_SLC0_TX_RST);
    REG_WRITE(SDIO_SLC0INT_CLR_REG, SDIO_SLC0_TX_SUC_EOF_INT | SDIO_SLC0_TX_DSCR_ERR_INT | SDIO_SLC0_TX_DSCR_EMPTY_INT);
    s_sip->slc->first_desc[0] = NULL;
    s_sip->slc->last_desc[0] = NULL;
    s_rx_armed = false;
}

int stub_target_sdio_rearm(uint8_t *buf, size_t max_size)
{
    if (s_rx_armed) {
        return STUB_LIB_OK;
    }

    if (s_rx_frame_ready) {
        return STUB_LIB_FAIL;
    }

    size_t rx_desc_count = max_size / SDIO_RX_BLKSZ;
    if (rx_desc_count > SDIO_RX_NUM_DESC) {
        rx_desc_count = SDIO_RX_NUM_DESC;
    }
    if (rx_desc_count == 0) {
        return STUB_LIB_ERR_INVALID_ARG;
    }

    size_t desc_offset = 0;

    for (size_t i = 0; i < rx_desc_count; i++) {
        lldesc_t *desc = &s_rx_desc[i];
        desc->size = SDIO_RX_BLKSZ;
        desc->length = 0;
        desc->offset = 0;
        desc->sosf = 0;
        desc->eof = 0;
        desc->owner = 1;
        desc->buf = buf + desc_offset;
        STAILQ_NEXT(desc, qe) = i + 1U < rx_desc_count ? &s_rx_desc[i + 1U] : NULL;

        desc_offset += SDIO_RX_BLKSZ;
    }

    REG_WRITE(SDIO_SLC0TX_LINK_ADDR_REG, (uint32_t)(uintptr_t)&s_rx_desc[0]);
    REG_SET_BIT(SDIO_SLC0TX_LINK_REG, SDIO_SLC0_TXLINK_START);
    sdio_set_rx_credits(rx_desc_count);

    s_rx_armed = true;
    return STUB_LIB_OK;
}

/* -------------------------------------------------------------------------
 * Process one received raw packet. head and known_tail come directly from
 * slc_from_host_chain_fetch(), which returns the complete host FIFO write.
 * ------------------------------------------------------------------------- */
static void sdio_process_from_host(lldesc_t *head, lldesc_t *known_tail)
{
    s_rx_received_len = 0;
    for (lldesc_t *cur = head; cur != NULL; cur = STAILQ_NEXT(cur, qe)) {
        size_t chunk_len = cur->length;

        s_rx_received_len += chunk_len;
        if (cur == known_tail) {
            break;
        }
    }

    s_rx_frame_ready = true;
    sdio_reset_from_host_window();
}

/* -------------------------------------------------------------------------
 * SLC0 interrupt service routine — handles all host→stub DMA events.
 * ------------------------------------------------------------------------- */
static void sdio_slc_isr(void)
{
    uint32_t raw = REG_READ(SDIO_SLC0INT_RAW_REG);

    if (raw & SDIO_SLC0_TX_SUC_EOF_INT) {
        REG_WRITE(SDIO_SLC0INT_CLR_REG, SDIO_SLC0_TX_SUC_EOF_INT);

        lldesc_t *head, *tail;
        slc_from_host_chain_fetch(&head, &tail);
        if (head == NULL) {
            s_rx_armed = false;
        } else {
            tail->eof = 0;
            sdio_process_from_host(head, tail);
        }
    }

    if (raw & SDIO_SLC0_TX_DSCR_ERR_INT) {
        REG_WRITE(SDIO_SLC0INT_CLR_REG, SDIO_SLC0_TX_DSCR_ERR_INT);
    }
    if (raw & SDIO_SLC0_RX_DSCR_ERR_INT) {
        REG_WRITE(SDIO_SLC0INT_CLR_REG, SDIO_SLC0_RX_DSCR_ERR_INT);
    }
    if (raw & SDIO_SLC0_TX_DSCR_EMPTY_INT) {
        REG_WRITE(SDIO_SLC0INT_CLR_REG, SDIO_SLC0_TX_DSCR_EMPTY_INT);
        sdio_reset_from_host_window();
    }
}

bool stub_target_sdio_take_rx_frame(size_t *out_len)
{
    if (!s_rx_frame_ready) {
        return false;
    }
    if (out_len) {
        *out_len = s_rx_received_len;
    }
    s_rx_frame_ready = false;
    return true;
}

bool stub_target_sdio_is_active(void)
{
    /* Eco version below 2 does not support SDIO download. */
    return _rom_eco_version >= 2 && sip_download_begin();
}

void stub_target_sdio_init(void)
{
    /* Reset only the host→slave DMA (our receive path, bit 0).
     * Do NOT reset the slave→host DMA (bit 1): sip_download_begin() already
     * configured it with the correct SDIO block-size and mode registers; a
     * reset would destroy that state and break subsequent RXLINK_START calls. */
    s_sip = sip_get_ptr();

    sdio_reset_from_host_window();
    REG_SET_BIT(SDIO_SLC_RX_DSCR_CONF_REG, SDIO_SLC0_TOKEN_NO_REPLACE);
    sdio_set_rx_credits(0);

    s_rx_armed = false;
    s_rx_frame_ready = false;
    s_rx_received_len = 0;

    /* Disable SLC interrupts before re-arming DMA so no interrupt fires
     * into an unregistered handler during the setup sequence. */
    REG_WRITE(SDIO_SLC0INT_ENA_REG, 0);

    REG_WRITE(SDIO_SLC0INT_CLR_REG, SDIO_SLC_INT_CLR_ALL);

    /* ESP32-C5 uses CLIC: the map register takes (cpu_intr_num + CLIC_EXT_INTR_NUM_OFFSET). */
    WRITE_PERI_REG(INTERRUPT_CORE0_SLC0_INTR_MAP_REG, SDIO_SLC_INTERRUPT_NUM + CLIC_EXT_INTR_NUM_OFFSET);
    esp_rom_esprv_intc_int_set_priority(SDIO_SLC_INTERRUPT_NUM, 1);
    esp_rom_isr_attach(SDIO_SLC_INTERRUPT_NUM, (void *)sdio_slc_isr, NULL);
    esp_rom_isr_unmask(BIT(SDIO_SLC_INTERRUPT_NUM));

    REG_WRITE(SDIO_SLC0INT_ENA_REG, SDIO_SLC0_TX_SUC_EOF_INT | SDIO_SLC0_TX_DSCR_ERR_INT | SDIO_SLC0_TX_DSCR_EMPTY_INT);
}

static bool sdio_wait_tx_done(void)
{
    uint64_t remaining_us = SDIO_TX_TIMEOUT_US;
    int wait_res = stub_target_wait_reg_bit_set(SDIO_SLC0INT_RAW_REG,
                                                SDIO_SLC0_RX_EOF_INT | SDIO_SLC0_RX_DSCR_ERR_INT,
                                                &remaining_us);
    if (wait_res != STUB_LIB_OK) {
        return false;
    }

    uint32_t raw = REG_READ(SDIO_SLC0INT_RAW_REG);
    if (raw & SDIO_SLC0_RX_EOF_INT) {
        REG_WRITE(SDIO_SLC0INT_CLR_REG, SDIO_SLC0_RX_DONE_INT | SDIO_SLC0_RX_EOF_INT | SDIO_SLC0_RX_DSCR_EMPTY_INT);
        lldesc_t *head, *tail;
        slc_to_host_chain_recycle(&head, &tail);
        return true;
    }
    if (raw & SDIO_SLC0_RX_DSCR_ERR_INT) {
        REG_WRITE(SDIO_SLC0INT_CLR_REG, SDIO_SLC0_RX_DSCR_ERR_INT);
    }

    return false;
}

int stub_target_sdio_tx_frame(const void *data, size_t len)
{
    /* DMA descriptor size/length fields are 14-bit bitfields. */
    s_tx_desc.size = (uint32_t)len & SDIO_DMA_DESC_MAX_LEN;
    s_tx_desc.length = (uint32_t)len & SDIO_DMA_DESC_MAX_LEN;
    s_tx_desc.offset = 0;
    s_tx_desc.sosf = 0;
    s_tx_desc.eof = 1;
    s_tx_desc.owner = 1;
    s_tx_desc.buf = (uint8_t *)data;
    STAILQ_NEXT(&s_tx_desc, qe) = NULL;

    REG_WRITE(SDIO_SLC0INT_CLR_REG,
              SDIO_SLC0_RX_DONE_INT | SDIO_SLC0_RX_EOF_INT | SDIO_SLC0_RX_DSCR_EMPTY_INT | SDIO_SLC0_RX_DSCR_ERR_INT);
    REG_WRITE(SLCHOST_CONF_W0_REG, (uint32_t)len);
    slc_send_to_host_chain(&s_tx_desc, &s_tx_desc);

    bool ok = sdio_wait_tx_done();

    /* Our descriptors are static, not ROM-owned SIP buffers. */
    SIP_FIRST_FREE_TO_HOST_DS(s_sip) = NULL;
    SIP_LAST_FREE_TO_HOST_DS(s_sip) = NULL;
    return ok ? STUB_LIB_OK : STUB_LIB_FAIL;
}
