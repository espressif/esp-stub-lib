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

extern int esp_rom_spiflash_config_readmode(int mode, bool legacy);
extern void SelectSpiFunction(uint32_t ishspi);
extern void spi_common_set_flash_cs_timing(void);
extern void spi_cache_mode_switch(uint32_t modebit);
extern void esp_rom_spiflash_attach(uint32_t ishspi, bool legacy);

extern esp_rom_spiflash_legacy_data_t *rom_spiflash_legacy_data;

/* SPI0 cache read-path helpers used to switch to 32-bit flash addressing */
extern void esp_rom_spi_set_op_mode(int spi_num, spi_flash_mode_t mode);
extern void esp_rom_spi_set_address_bit_len(int spi_num, int addr_bits);

/* ECO version from ROM - used to route to correct ROM functions */
extern uint32_t _rom_eco_version;
/* ECO-specific ROM function declarations */
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

void stub_target_opiflash_exec_cmd(const opiflash_cmd_params_t *params)
{
    if (_rom_eco_version >= 3) {
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
    } else {
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
    }
}

void stub_target_spi_wait_ready(void)
{
    while (REG_GET_FIELD(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_MST_ST)) {
        /* busy wait */
    }
}

uint32_t stub_target_get_max_supported_flash_size(void)
{
    /* ESP32-C5 supports up to 32MB with 4-byte addressing */
    return MIB(32);
}

/*
 * SPI0 cache read settings for a 32-bit address FAST_READ (cmd 0x0C).
 * Mirrors the ESP32-S3 stub path: FAST_READ_4B + 8 dummy cycles. A SLOWRD
 * (0x13, 0 dummy) read comes back bit-shifted through the SPI0 cache at the
 * flash clocks IDF leaves configured, so FAST_READ with a dummy phase is used.
 */
#define ADDR32_CACHE_CMD          CMD_FSTRD4B
#define ADDR32_CACHE_CMD_BITLEN   8
#define ADDR32_CACHE_ADDR_BITLEN  32
#define ADDR32_CACHE_DUMMY_BITLEN 8

static struct {
    bool applied;
    uint32_t saved_user;
    uint32_t saved_user1;
    uint32_t saved_user2;
    uint32_t saved_ctrl;
    uint32_t saved_ddr;
    uint32_t saved_cache_fctrl;
    uint32_t saved_timing_cali;
    uint8_t saved_dummy_len_plus;
} s_addr32_state;

static void enable_4byte_cache_mode(void)
{
    if (REG_GET_FIELD(SPI_MEM_USER1_REG(FLASH_SPI_NUM_INT), SPI_MEM_USR_ADDR_BITLEN) == ADDR32_CACHE_ADDR_BITLEN - 1) {
        return;
    }

    s_addr32_state.saved_user = REG_READ(SPI_MEM_USER_REG(FLASH_SPI_NUM_INT));
    s_addr32_state.saved_user1 = REG_READ(SPI_MEM_USER1_REG(FLASH_SPI_NUM_INT));
    s_addr32_state.saved_user2 = REG_READ(SPI_MEM_USER2_REG(FLASH_SPI_NUM_INT));
    s_addr32_state.saved_ctrl = REG_READ(SPI_MEM_CTRL_REG(FLASH_SPI_NUM_INT));
    s_addr32_state.saved_ddr = REG_READ(SPI_MEM_DDR_REG(FLASH_SPI_NUM_INT));
    s_addr32_state.saved_cache_fctrl = REG_READ(SPI_MEM_CACHE_FCTRL_REG(FLASH_SPI_NUM_INT));
    s_addr32_state.saved_timing_cali = REG_READ(SPI_MEM_TIMING_CALI_REG(FLASH_SPI_NUM_INT));
    s_addr32_state.saved_dummy_len_plus = rom_spiflash_legacy_data->dummy_len_plus[FLASH_SPI_NUM_INT];

    /* Clear IDF's extra-dummy tuning on SPI0; otherwise it stacks on top of the
     * dummy count we program below and mis-times the read. */
    CLEAR_PERI_REG_MASK(SPI_MEM_TIMING_CALI_REG(FLASH_SPI_NUM_INT), SPI_MEM_TIMING_CALI_M);
    REG_SET_FIELD(SPI_MEM_TIMING_CALI_REG(FLASH_SPI_NUM_INT), SPI_MEM_EXTRA_DUMMY_CYCLELEN, 0);
    rom_spiflash_legacy_data->dummy_len_plus[FLASH_SPI_NUM_INT] = 0;

    STUB_LOGD("Switching SPI0 cache to 32-bit addr (cmd=0x%x, dummy=%u)\n",
              ADDR32_CACHE_CMD,
              ADDR32_CACHE_DUMMY_BITLEN);

    stub_lib_cache_stop();

    /* The C5 ROM does not export esp_rom_opiflash_cache_mode_config, so inline
     * the C5 branch of the patched ESP-IDF esp_rom_spiflash_cache_mode_config()
     * for a single-line FAST_READ with a 4-byte address phase. set_op_mode()
     * resets the read/write mode bits (CTRL/USER/DDR) and set_address_bit_len()
     * enables the SPI0 cache 4-byte address bit and the address phase. */
    esp_rom_spi_set_op_mode(FLASH_SPI_NUM_INT, SPI_FLASH_FASTRD_MODE);
    esp_rom_spi_set_address_bit_len(FLASH_SPI_NUM_INT, ADDR32_CACHE_ADDR_BITLEN);
    REG_SET_BIT(SPI_MEM_USER_REG(FLASH_SPI_NUM_INT), SPI_MEM_USR_DUMMY);
    REG_SET_FIELD(SPI_MEM_USER1_REG(FLASH_SPI_NUM_INT), SPI_MEM_USR_DUMMY_CYCLELEN, ADDR32_CACHE_DUMMY_BITLEN - 1);
    REG_SET_FIELD(SPI_MEM_USER2_REG(FLASH_SPI_NUM_INT), SPI_MEM_USR_COMMAND_VALUE, ADDR32_CACHE_CMD);
    REG_SET_FIELD(SPI_MEM_USER2_REG(FLASH_SPI_NUM_INT), SPI_MEM_USR_COMMAND_BITLEN, ADDR32_CACHE_CMD_BITLEN - 1);
    REG_SET_FIELD(SPI_MEM_DDR_REG(FLASH_SPI_NUM_INT), SPI_FMEM_VAR_DUMMY, 0);

    stub_lib_cache_start();

    s_addr32_state.applied = true;
}

static void disable_4byte_cache_mode(void)
{
    if (!s_addr32_state.applied) {
        return;
    }

    stub_lib_cache_stop();
    REG_WRITE(SPI_MEM_USER_REG(FLASH_SPI_NUM_INT), s_addr32_state.saved_user);
    REG_WRITE(SPI_MEM_USER1_REG(FLASH_SPI_NUM_INT), s_addr32_state.saved_user1);
    REG_WRITE(SPI_MEM_USER2_REG(FLASH_SPI_NUM_INT), s_addr32_state.saved_user2);
    REG_WRITE(SPI_MEM_CTRL_REG(FLASH_SPI_NUM_INT), s_addr32_state.saved_ctrl);
    REG_WRITE(SPI_MEM_DDR_REG(FLASH_SPI_NUM_INT), s_addr32_state.saved_ddr);
    REG_WRITE(SPI_MEM_CACHE_FCTRL_REG(FLASH_SPI_NUM_INT), s_addr32_state.saved_cache_fctrl);
    REG_WRITE(SPI_MEM_TIMING_CALI_REG(FLASH_SPI_NUM_INT), s_addr32_state.saved_timing_cali);
    rom_spiflash_legacy_data->dummy_len_plus[FLASH_SPI_NUM_INT] = s_addr32_state.saved_dummy_len_plus;
    stub_lib_cache_start();

    s_addr32_state.applied = false;
}

void stub_target_flash_set_4byte_cache_mode(bool enable)
{
    if (enable) {
        enable_4byte_cache_mode();
    } else {
        disable_4byte_cache_mode();
    }
}

uint32_t __attribute__((weak)) stub_target_flash_get_flash_id(void)
{
    esp_rom_spi_flash_update_id();
    return stub_target_flash_get_config()->flash_id;
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
