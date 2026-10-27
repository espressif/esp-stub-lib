/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>

#include <esp-stub-lib/cache.h>
#include <esp-stub-lib/log.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>
#include <target/flash.h>

#include <private/flash_commands.h>
#include <private/rom_flash_config.h>

#include <soc/spi_mem_compat.h>

typedef struct {
    uint8_t addr_bit_len;
    uint8_t dummy_bit_len;
    uint16_t cmd;
    uint8_t cmd_bit_len;
    uint8_t var_dummy_en;
} esp_rom_opiflash_spi0rd_t;

extern esp_rom_spiflash_legacy_data_t *rom_spiflash_legacy_data;
extern void esp_rom_spiflash_cache_mode_config(spi_flash_mode_t mode, const esp_rom_opiflash_spi0rd_t *cache);

extern void esp_rom_spiflash_attach(uint32_t ishspi, bool legacy);
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

void stub_target_opiflash_exec_cmd(const opiflash_cmd_params_t *params)
{
    esp_rom_opiflash_exec_cmd(params->spi_num,
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

void stub_target_spi_wait_ready(void)
{
    while (REG_GET_FIELD(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_MST_ST) ||
           REG_GET_FIELD(SPI_MEM_CMD_REG(FLASH_SPI_NUM), SPI_MEM_SLV_ST)) {
        /* busy wait */
    }
}

uint32_t stub_target_get_max_supported_flash_size(void)
{
    /* ESP32-S31 has a 64MB flash MMU address range and supports 4-byte addressing. */
    return MIB(64);
}

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
    /* Bootloader / IDF may already have 4-byte cache addressing (QIO/DIO +
     * timing). Leave that alone; reprogramming FASTRD here would desync
     * memory-mapped fetches until a full chip reset. Prefer the dedicated
     * CACHE_USR_ADDR_4BYTE bit; also accept USR_ADDR_BITLEN >= 31 for setups
     * that only programmed the address phase (incl. addr+mode-bit lengths). */
    if (REG_GET_BIT(SPI_MEM_C_CACHE_FCTRL_REG, SPI_MEM_C_CACHE_USR_ADDR_4BYTE) ||
        REG_GET_FIELD(SPI_MEM_C_USER1_REG, SPI_MEM_C_USR_ADDR_BITLEN) >= ADDR32_CACHE_ADDR_BITLEN - 1) {
        return;
    }

    s_addr32_state.saved_user = REG_READ(SPI_MEM_C_USER_REG);
    s_addr32_state.saved_user1 = REG_READ(SPI_MEM_C_USER1_REG);
    s_addr32_state.saved_user2 = REG_READ(SPI_MEM_C_USER2_REG);
    s_addr32_state.saved_ctrl = REG_READ(SPI_MEM_C_CTRL_REG);
    s_addr32_state.saved_ddr = REG_READ(SPI_MEM_C_DDR_REG);
    s_addr32_state.saved_cache_fctrl = REG_READ(SPI_MEM_C_CACHE_FCTRL_REG);
    s_addr32_state.saved_timing_cali = REG_READ(SPI_MEM_C_TIMING_CALI_REG);
    s_addr32_state.saved_dummy_len_plus = rom_spiflash_legacy_data->dummy_len_plus[FLASH_SPI_NUM_INT];

    /* Use FAST_READ_4B + 8 dummy; SLOWRD/0x13/0-dummy comes back bit-shifted
     * through SPI0 cache at typical flash clocks. */
    const esp_rom_opiflash_spi0rd_t cache_rd = {
        .addr_bit_len = ADDR32_CACHE_ADDR_BITLEN,
        .dummy_bit_len = ADDR32_CACHE_DUMMY_BITLEN,
        .cmd = ADDR32_CACHE_CMD,
        .cmd_bit_len = ADDR32_CACHE_CMD_BITLEN,
        .var_dummy_en = 0,
    };

    stub_lib_cache_stop();

    /* Clear IDF's extra-dummy tuning on SPI0; otherwise it stacks on top of
     * the dummy count passed to cache_mode_config() and mis-times reads. */
    CLEAR_PERI_REG_MASK(SPI_MEM_C_TIMING_CALI_REG, SPI_MEM_C_TIMING_CALI_M);
    REG_SET_FIELD(SPI_MEM_C_TIMING_CALI_REG, SPI_MEM_C_EXTRA_DUMMY_CYCLELEN, 0);
    REG_SET_BIT(SPI_MEM_C_TIMING_CALI_REG, SPI_MEM_C_TIMING_CALI_UPDATE);
    rom_spiflash_legacy_data->dummy_len_plus[FLASH_SPI_NUM_INT] = 0;

    esp_rom_spiflash_cache_mode_config(SPI_FLASH_FASTRD_MODE, &cache_rd);
    /* SPI0 cache only sends a 32-bit address with CACHE_USR_ADDR_4BYTE set. */
    REG_SET_BIT(SPI_MEM_C_CACHE_FCTRL_REG, SPI_MEM_C_CACHE_USR_ADDR_4BYTE);

    stub_lib_cache_start();

    s_addr32_state.applied = true;
}

static void disable_4byte_cache_mode(void)
{
    if (!s_addr32_state.applied)
        return;

    stub_lib_cache_stop();
    REG_WRITE(SPI_MEM_C_USER_REG, s_addr32_state.saved_user);
    REG_WRITE(SPI_MEM_C_USER1_REG, s_addr32_state.saved_user1);
    REG_WRITE(SPI_MEM_C_USER2_REG, s_addr32_state.saved_user2);
    REG_WRITE(SPI_MEM_C_CTRL_REG, s_addr32_state.saved_ctrl);
    REG_WRITE(SPI_MEM_C_DDR_REG, s_addr32_state.saved_ddr);
    REG_WRITE(SPI_MEM_C_CACHE_FCTRL_REG, s_addr32_state.saved_cache_fctrl);
    REG_WRITE(SPI_MEM_C_TIMING_CALI_REG, s_addr32_state.saved_timing_cali);
    REG_SET_BIT(SPI_MEM_C_TIMING_CALI_REG, SPI_MEM_C_TIMING_CALI_UPDATE);
    rom_spiflash_legacy_data->dummy_len_plus[FLASH_SPI_NUM_INT] = s_addr32_state.saved_dummy_len_plus;
    stub_lib_cache_start();

    s_addr32_state.applied = false;
}

void stub_target_flash_set_4byte_cache_mode(bool enable)
{
    if (enable)
        enable_4byte_cache_mode();
    else
        disable_4byte_cache_mode();
}

bool stub_target_flash_needs_attach(void)
{
    /* cache_is_enabled() is false when SPI0 has no read command (cold after
     * stall-through-SYS_SW_RST) or when cache/MMU are down — both need attach. */
    return !stub_target_cache_is_enabled();
}

void stub_target_flash_init(void *state, stub_lib_flash_attach_policy_t attach_policy)
{
    (void)state;

    if (attach_policy == STUB_LIB_FLASH_ATTACH_ALWAYS || stub_target_flash_needs_attach()) {
        STUB_LOGD("Attach spi flash...\n");
        esp_rom_spiflash_attach(0, false);
    }

    REG_SET_BIT(SPI_MEM_USER_REG(FLASH_SPI_NUM), SPI_MEM_USR_COMMAND);
}
