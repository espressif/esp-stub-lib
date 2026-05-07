/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * Type definitions for the ROM SLC/SIP layer used by SDIO.
 *
 * The struct layouts here must match the ROM's internal structures exactly.
 * Layout is derived from static analysis of the ESP32-C6 ROM and confirmed
 * to match ESP32-C5 (same SIP/SLC ROM API, same struct layout).
 *   - lldesc_t: standard ESP DMA linked-list descriptor (12 bytes)
 *   - sip_t:    ROM SIP control block (~88 bytes)
 *
 * STAILQ_HEAD is assumed to be the standard BSD two-pointer form (8 bytes on
 * a 32-bit system). enum/sip_state_t is int-sized (4 bytes).
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/* -------------------------------------------------------------------------
 * DMA linked-list descriptor
 * ------------------------------------------------------------------------- */

typedef struct lldesc_s {
    volatile uint32_t size : 14;   /* buffer capacity in bytes */
    volatile uint32_t length : 14; /* valid bytes (set by HW on RX, SW on TX) */
    volatile uint32_t offset : 1;
    volatile uint32_t sosf : 1;  /* start-of-sub-frame */
    volatile uint32_t eof : 1;   /* last descriptor in packet */
    volatile uint32_t owner : 1; /* 0 = CPU, 1 = DMA */
    volatile uint8_t *buf;       /* pointer to payload buffer */
    union {
        volatile uint32_t empty;
        struct {
            struct lldesc_s *stqe_next;
        } qe; /* next descriptor */
    };
} lldesc_t;

#define STAILQ_NEXT(elm, field) ((elm)->field.stqe_next)

/* -------------------------------------------------------------------------
 * SIP control structure (ROM-owned, accessed via sip_get_ptr())
 *
 * Offsets (32-bit pointers):
 *   0  blksz_evt (2)
 *   2  blksz_cmd (2)
 *   4  blksz_from_host_data (2)
 *   6  blksz_to_host_data (2)
 *   8  free_evtq [STAILQ_HEAD = 2 x ptr = 8 bytes]
 *  16  flags (4)
 *  20  first_to_host_free (4)
 *  24  last_to_host_free (4)
 *  28  first_from_host (4)
 *  32  last_from_host (4)
 *  36  from_host_seq (4)
 *  40  to_host_seq (4)
 *  44  state (4, int-sized enum)
 *  48  slc (4)
 *  52  ext (4)
 *  56  first_to_host_pend (4)
 *  60  last_to_host_pend (4)
 *  64  to_host_length (4)
 *  68  sending_to_host (1) + pad (3)
 *  72  to_host_data_done_cb fn ptr (4)
 *  76  to_host_ampduEntryBlock_done_cb fn ptr (4)
 *  80  to_host_done_process fn ptr (4)
 *  84  from_host_process fn ptr (4)
 *  88  get_send_length fn ptr (4)
 *  92  write_memory (4)
 * ------------------------------------------------------------------------- */

/* Minimal ROM SLC control block prefix.
 * The ROM recycle helper uses first_desc[0] / last_desc[0] for FROM_HOST. */
typedef struct slc_ctrl_s {
    uint16_t blksz_cmd;
    uint16_t _pad0;
    lldesc_t *first_desc[2];
    lldesc_t *last_desc[2];
} slc_ctrl_t;

/* Free event descriptor queue head - STAILQ_HEAD(sip_qh, lldesc_s) */
struct sip_evtq_head {
    lldesc_t *stqh_first;
    lldesc_t **stqh_last;
};

typedef uint32_t sip_state_t;

typedef struct sip_s {
    uint16_t blksz_evt;
    uint16_t blksz_cmd;
    uint16_t blksz_from_host_data;
    uint16_t blksz_to_host_data;
    struct sip_evtq_head free_evtq; /* 8 bytes */
    uint32_t flags;
    lldesc_t *first_to_host_free;
    lldesc_t *last_to_host_free;
    lldesc_t *first_from_host;
    lldesc_t *last_from_host;
    uint32_t from_host_seq;
    uint32_t to_host_seq;
    sip_state_t state;
    slc_ctrl_t *slc;
    void *ext;
    lldesc_t *first_to_host_pend;
    lldesc_t *last_to_host_pend;
    uint32_t to_host_length;
    bool sending_to_host;
    uint8_t _pad[3];
    void (*to_host_data_done_cb)(lldesc_t *head, lldesc_t *tail, uint16_t num);
    void (*to_host_ampduEntryBlock_done_cb)(lldesc_t *head, lldesc_t *tail, uint16_t num);
    void (*to_host_done_process)(uint32_t par);
    void (*from_host_process)(uint32_t par);
    uint32_t (*get_send_length)(lldesc_t *head, lldesc_t **res);
    uint32_t write_memory;
} sip_t;

/* Field accessor macros */
#define SIP_FIRST_FREE_TO_HOST_DS(sip) ((sip)->first_to_host_free)
#define SIP_LAST_FREE_TO_HOST_DS(sip)  ((sip)->last_to_host_free)
#define SIP_FIRST_FROM_HOST_DS(sip)    ((sip)->first_from_host)
#define SIP_LAST_FROM_HOST_DS(sip)     ((sip)->last_from_host)
#define SIP_FIRST_PEND_TO_HOST_DS(sip) ((sip)->first_to_host_pend)
#define SIP_LAST_PEND_TO_HOST_DS(sip)  ((sip)->last_to_host_pend)
