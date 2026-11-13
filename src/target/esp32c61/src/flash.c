/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/cache.h>
#include <esp-stub-lib/log.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>
#include <target/flash.h>

#include <private/flash_commands.h>
#include <private/rom_flash_config.h>

#include <soc/spi_mem_compat.h>

/* ECO version from ROM - used to route to correct ROM functions */
extern uint32_t _rom_eco_version;
extern esp_rom_spiflash_legacy_data_t *rom_spiflash_legacy_data;
extern void esp_rom_spi_set_op_mode(int spi_num, spi_flash_mode_t mode);
extern void esp_rom_spi_set_address_bit_len(int spi_num, int addr_bits);

extern void esp_rom_opiflash_exec_cmd(int spi_num,
                                      spi_flash_mode_t mode,
                                      uint32_t cmd,
                                      int cmd_bit_len,
                                      uint32_t addr,
                                      int addr_bit_len,
                                      int dummy_bits,
                                      const uint8_t *mosi_data,
                                      int mosi_bit_len,
                                      uint8_t *miso_data,
                                      int miso_bit_len,
                                      uint32_t cs_mask,
                                      bool is_write_erase_operation);

extern void esp_rom_opiflash_exec_cmd_eco0(int spi_num,
                                           spi_flash_mode_t mode,
                                           uint32_t cmd,
                                           int cmd_bit_len,
                                           uint32_t addr,
                                           int addr_bit_len,
                                           int dummy_bits,
                                           const uint8_t *mosi_data,
                                           int mosi_bit_len,
                                           uint8_t *miso_data,
                                           int miso_bit_len,
                                           uint32_t cs_mask,
                                           bool is_write_erase_operation);

extern void esp_rom_opiflash_exec_cmd_eco2(int spi_num,
                                           spi_flash_mode_t mode,
                                           uint32_t cmd,
                                           int cmd_bit_len,
                                           uint32_t addr,
                                           int addr_bit_len,
                                           int dummy_bits,
                                           const uint8_t *mosi_data,
                                           int mosi_bit_len,
                                           uint8_t *miso_data,
                                           int miso_bit_len,
                                           uint32_t cs_mask,
                                           bool is_write_erase_operation);

extern void esp_rom_opiflash_exec_cmd_eco3(int spi_num,
                                           spi_flash_mode_t mode,
                                           uint32_t cmd,
                                           int cmd_bit_len,
                                           uint32_t addr,
                                           int addr_bit_len,
                                           int dummy_bits,
                                           const uint8_t *mosi_data,
                                           int mosi_bit_len,
                                           uint8_t *miso_data,
                                           int miso_bit_len,
                                           uint32_t cs_mask,
                                           bool is_write_erase_operation);

extern void esp_rom_opiflash_exec_cmd_eco4(int spi_num,
                                           spi_flash_mode_t mode,
                                           uint32_t cmd,
                                           int cmd_bit_len,
                                           uint32_t addr,
                                           int addr_bit_len,
                                           int dummy_bits,
                                           const uint8_t *mosi_data,
                                           int mosi_bit_len,
                                           uint8_t *miso_data,
                                           int miso_bit_len,
                                           uint32_t cs_mask,
                                           bool is_write_erase_operation);

extern void esp_rom_opiflash_exec_cmd_eco5(int spi_num,
                                           spi_flash_mode_t mode,
                                           uint32_t cmd,
                                           int cmd_bit_len,
                                           uint32_t addr,
                                           int addr_bit_len,
                                           int dummy_bits,
                                           const uint8_t *mosi_data,
                                           int mosi_bit_len,
                                           uint8_t *miso_data,
                                           int miso_bit_len,
                                           uint32_t cs_mask,
                                           bool is_write_erase_operation);

/*
 * Unknown newer revisions fall back to the newest known ROM function.
 * ECO5 reports _rom_eco_version as 0x14 (not 5); keep the threshold at 0x14.
 */
void stub_target_opiflash_exec_cmd(const opiflash_cmd_params_t *params)
{
    if (_rom_eco_version >= 0x14) {
        esp_rom_opiflash_exec_cmd_eco5(params->spi_num,
                                       params->mode,
                                       params->cmd,
                                       params->cmd_bit_len,
                                       params->addr,
                                       params->addr_bit_len,
                                       params->dummy_bits,
                                       params->mosi_data,
                                       params->mosi_bit_len,
                                       params->miso_data,
                                       params->miso_bit_len,
                                       params->cs_mask,
                                       params->is_write_erase_operation);
    } else if (_rom_eco_version == 4) {
        esp_rom_opiflash_exec_cmd_eco4(params->spi_num,
                                       params->mode,
                                       params->cmd,
                                       params->cmd_bit_len,
                                       params->addr,
                                       params->addr_bit_len,
                                       params->dummy_bits,
                                       params->mosi_data,
                                       params->mosi_bit_len,
                                       params->miso_data,
                                       params->miso_bit_len,
                                       params->cs_mask,
                                       params->is_write_erase_operation);
    } else if (_rom_eco_version == 3) {
        esp_rom_opiflash_exec_cmd_eco3(params->spi_num,
                                       params->mode,
                                       params->cmd,
                                       params->cmd_bit_len,
                                       params->addr,
                                       params->addr_bit_len,
                                       params->dummy_bits,
                                       params->mosi_data,
                                       params->mosi_bit_len,
                                       params->miso_data,
                                       params->miso_bit_len,
                                       params->cs_mask,
                                       params->is_write_erase_operation);
    } else if (_rom_eco_version == 2) {
        esp_rom_opiflash_exec_cmd_eco2(params->spi_num,
                                       params->mode,
                                       params->cmd,
                                       params->cmd_bit_len,
                                       params->addr,
                                       params->addr_bit_len,
                                       params->dummy_bits,
                                       params->mosi_data,
                                       params->mosi_bit_len,
                                       params->miso_data,
                                       params->miso_bit_len,
                                       params->cs_mask,
                                       params->is_write_erase_operation);
    } else {
        /* ECO0/1 */
        esp_rom_opiflash_exec_cmd_eco0(params->spi_num,
                                       params->mode,
                                       params->cmd,
                                       params->cmd_bit_len,
                                       params->addr,
                                       params->addr_bit_len,
                                       params->dummy_bits,
                                       params->mosi_data,
                                       params->mosi_bit_len,
                                       params->miso_data,
                                       params->miso_bit_len,
                                       params->cs_mask,
                                       params->is_write_erase_operation);
    }
}

void stub_target_spi_wait_ready(void)
{
    while (REG_GET_FIELD(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_MST_ST) ||
           REG_GET_FIELD(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_SLV_ST)) {
        /* busy wait */
    }
}

uint32_t stub_target_get_max_supported_flash_size(void)
{
    /* ESP32-C61 supports up to 32MB with 4-byte addressing */
    return MIB(32);
}

#define ADDR32_CACHE_CMD          CMD_FSTRD4B
#define ADDR32_CACHE_CMD_BITLEN   8
#define ADDR32_CACHE_ADDR_BITLEN  32
#define ADDR32_CACHE_DUMMY_BITLEN 8

static bool psram_spi0_cache_active(void)
{
    uint32_t sctrl = REG_READ(SPI_MEM_CACHE_SCTRL_REG(FLASH_SPI_NUM_INT));

    return (sctrl & (SPI_MEM_USR_SRAM_DIO | SPI_MEM_USR_SRAM_QIO)) != 0;
}

static void enable_4byte_cache_mode(void)
{
    /* If SPI0 cache already uses a 32-bit address phase (bootloader / IDF set
     * this up for apps in the upper 16 MiB), leave the existing QIO/DIO +
     * timing config alone. Reprogramming FASTRD here would desync
     * memory-mapped fetches until a full chip reset re-inits MSPI. */
    if (REG_GET_FIELD(SPI_MEM_USER1_REG(FLASH_SPI_NUM_INT), SPI_MEM_USR_ADDR_BITLEN) == ADDR32_CACHE_ADDR_BITLEN - 1) {
        return;
    }

    /* Forcing single-line FAST_READ_4B while PSRAM shares MSPI in DIO/QIO
     * desyncs flash timing and corrupts the shared I/D cache (Load access
     * fault after flash SW BP install). Apps that keep 24-bit SPI0 but run on
     * a >16MB part only need low-half flash access for typical SW BPs — skip. */
    if (psram_spi0_cache_active()) {
        STUB_LOGI("Skipping SPI0 FASTRD_4B switch: PSRAM cache active (DIO/QIO)\n");
        return;
    }

    /* Flush dirty PSRAM lines while MSPI is still in IDF's DIO/QIO + timing. */
    stub_lib_cache_writeback_all();

    STUB_LOGD("Switching SPI0 cache to 32-bit addr (cmd=0x%x, dummy=%u)\n",
              ADDR32_CACHE_CMD,
              ADDR32_CACHE_DUMMY_BITLEN);

    stub_lib_cache_stop();

    /* Clear IDF's extra-dummy tuning on SPI0; otherwise it stacks on top of the
     * dummy count programmed below and mis-times reads. */
    CLEAR_PERI_REG_MASK(SPI_MEM_TIMING_CALI_REG(FLASH_SPI_NUM_INT), SPI_MEM_TIMING_CALI_M);
    REG_SET_FIELD(SPI_MEM_TIMING_CALI_REG(FLASH_SPI_NUM_INT), SPI_MEM_EXTRA_DUMMY_CYCLELEN, 0);
    rom_spiflash_legacy_data->dummy_len_plus[FLASH_SPI_NUM_INT] = 0;

    /* The C61 ROM has no esp_rom_opiflash_cache_mode_config(), so program SPI0
     * directly: single-line FAST_READ_4B with 8 dummy cycles. */
    esp_rom_spi_set_op_mode(FLASH_SPI_NUM_INT, SPI_FLASH_FASTRD_MODE);
    esp_rom_spi_set_address_bit_len(FLASH_SPI_NUM_INT, ADDR32_CACHE_ADDR_BITLEN);
    REG_SET_BIT(SPI_MEM_USER_REG(FLASH_SPI_NUM_INT), SPI_MEM_USR_DUMMY);
    REG_SET_FIELD(SPI_MEM_USER1_REG(FLASH_SPI_NUM_INT), SPI_MEM_USR_DUMMY_CYCLELEN, ADDR32_CACHE_DUMMY_BITLEN - 1);
    REG_SET_FIELD(SPI_MEM_USER2_REG(FLASH_SPI_NUM_INT), SPI_MEM_USR_COMMAND_VALUE, ADDR32_CACHE_CMD);
    REG_SET_FIELD(SPI_MEM_USER2_REG(FLASH_SPI_NUM_INT), SPI_MEM_USR_COMMAND_BITLEN, ADDR32_CACHE_CMD_BITLEN - 1);
    REG_SET_FIELD(SPI_MEM_DDR_REG(FLASH_SPI_NUM_INT), SPI_FMEM_VAR_DUMMY, 0);

    stub_lib_cache_start();

    /* Drop lines that may have been filled under FASTRD_4B without IDF timing. */
    stub_lib_cache_invalidate_all();
}

void stub_target_flash_set_4byte_cache_mode(bool enable)
{
    /* SPI0 is intentionally left in FAST_READ_4B mode (without timing tuning)
     * until the next chip reset: the resumed application may execute from IROM
     * pages above 16 MiB. */
    if (enable) {
        enable_4byte_cache_mode();
    }
}

bool stub_target_flash_needs_attach(void)
{
    return !stub_target_cache_is_enabled();
}

void stub_target_flash_init(void *state, stub_lib_flash_attach_policy_t attach_policy)
{
    (void)state;

    if (attach_policy == STUB_LIB_FLASH_ATTACH_ALWAYS || stub_target_flash_needs_attach()) {
        STUB_LOGD("Attach spi flash...\n");
        stub_target_flash_attach(0, 0);
    }

    REG_SET_BIT(SPI_MEM_USER_REG(1), SPI_MEM_USR_COMMAND);
}
