/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SDMMC_SECTOR_SIZE           512U

// Error codes returned by SDMMC functions
#define SDMMC_ERR_NOT_INITIALIZED   (-1)
#define SDMMC_ERR_NO_CARD           (-2)
#define SDMMC_ERR_TIMEOUT           (-3)
#define SDMMC_ERR_CMD_FAIL          (-4)
#define SDMMC_ERR_DATA_FAIL         (-5)
#define SDMMC_ERR_UNSUPPORTED       (-6)
#define SDMMC_ERR_BAD_ARG           (-7)
#define SDMMC_ERR_PIN_INVALID       (-8)

// Diagnostic stage codes (see stub_target_sdmmc_get_last_diag).  Real SD/MMC
// command indices are 0..63 so 0xFC..0xFF are unambiguous sentinels.
#define SDMMC_DIAG_STAGE_NONE       0xFFU // no command issued yet
#define SDMMC_DIAG_STAGE_UPDATE_CLK 0xFEU // update_clk_reg pseudo-command
#define SDMMC_DIAG_STAGE_WAIT_BUSY  0xFDU // polling STATUS.data_busy after R1b
#define SDMMC_DIAG_STAGE_DATA_XFER  0xFCU // CMD17/24 data transfer

// Attach configuration passed by the host (16 bytes, little-endian on the wire).
// Set slot=0xFF to select the chip-default pin map (S3: CLK=14, CMD=15,
// D0..D7 = 2,4,12,13,33,34,35,36; slot=0; width=4; 20 MHz).
typedef struct {
    uint8_t slot;      // 0 or 1; 0xFF triggers chip defaults
    uint8_t width;     // 1, 4, or 8
    uint16_t freq_khz; // post-init bus frequency (0 = default 20 MHz)
    uint8_t cd_pin;    // card-detect GPIO (0xFF = none)
    uint8_t wp_pin;    // write-protect GPIO (0xFF = none)
    uint8_t pin_clk;
    uint8_t pin_cmd;
    uint8_t pin_d[8]; // entries past width-1 are ignored
} stub_target_sdmmc_attach_config_t;

typedef struct {
    uint32_t ocr;             // OCR register (R3 response)
    uint32_t cid[4];          // CID raw (R2)
    uint32_t csd[4];          // CSD raw (R2)
    uint32_t rca;             // relative card address
    uint64_t capacity_bytes;  // 0 if unknown (e.g. SDSC)
    uint8_t is_mmc;           // 1 if eMMC (CMD1 init path)
    uint8_t is_high_capacity; // 1 for SDHC/SDXC/eMMC (LBA addressing)
    uint8_t width;            // actual bus width selected (1/4/8)
    uint8_t reserved;
} stub_target_sdmmc_card_info_t;

/**
 * @brief Initialise SDMMC host, route pins, and bring up the card.
 * @param cfg Attach configuration; NULL for chip defaults.
 * @return 0 on success, negative SDMMC_ERR_* on failure.
 */
int stub_target_sdmmc_attach(const stub_target_sdmmc_attach_config_t *cfg);

/**
 * @brief Return card info populated by the last successful attach, or NULL.
 */
const stub_target_sdmmc_card_info_t *stub_target_sdmmc_get_card_info(void);

/**
 * @brief Read one 512-byte sector by LBA.
 * @param lba Sector index.
 * @param buf Destination, 4-byte aligned, in internal DRAM (IDMAC can't reach PSRAM).
 * @return 0 on success, negative SDMMC_ERR_* on failure.
 */
int stub_target_sdmmc_read_sector(uint32_t lba, uint8_t *buf);

/**
 * @brief Write one 512-byte sector by LBA.  buf must be 4-byte aligned.
 */
int stub_target_sdmmc_write_sector(uint32_t lba, const uint8_t *buf);

/**
 * @brief Erase an LBA range via CMD32/33/38 (SD) or CMD35/36/38 (eMMC).
 *        Both endpoints inclusive.
 */
int stub_target_sdmmc_erase_range(uint32_t start_lba, uint32_t end_lba);

/**
 * @brief Return the CMD index and RINTSTS snapshot captured at the most recent
 *        failure, for plugin handlers to surface in the response value.
 */
void stub_target_sdmmc_get_last_diag(uint8_t *cmd_idx_out, uint32_t *rintsts_out);

#ifdef __cplusplus
}
#endif
