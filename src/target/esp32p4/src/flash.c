/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/cache.h>
#include <esp-stub-lib/log.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/cache.h>
#include <target/clock.h>
#include <target/flash.h>

#include <private/flash_commands.h>
#include <private/rom_flash_config.h>

#include <soc/efuse_reg.h>
#include <soc/hp_sys_clkrst_reg.h>
#include <soc/pmu_reg.h>
#include <soc/spi_mem_compat.h>

extern uint32_t esp_rom_get_xtal_freq(void);
extern esp_rom_spiflash_legacy_data_t *rom_spiflash_legacy_data;
extern void esp_rom_spi_set_op_mode(int spi_num, spi_flash_mode_t mode);
extern void esp_rom_spi_set_address_bit_len(int spi_num, int addr_bits);

/* ECO version from ROM - used to route to correct ROM functions */
extern uint32_t _rom_eco_version;

extern void esp_rom_spiflash_attach(uint32_t ishspi, bool legacy);
extern void esp_rom_spiflash_boot_attach(uint32_t ishspi, bool legacy, bool boot_mode);

void stub_target_reset_default_spi_pins(void)
{
    /* ESP32-P4 uses dedicated pins for SPI flash. */
}

uint32_t stub_target_flash_get_src_freq_mhz(void)
{
    /* XTAL, 480 MHz SPLL or 400 MHz CPLL, the unused value as the fastest source */
    static const uint16_t src_mhz[] = { 480, 400, 480 };
    uint32_t sel = REG_GET_FIELD(HP_SYS_CLKRST_PERI_CLK_CTRL00_REG, HP_SYS_CLKRST_REG_FLASH_CLK_SRC_SEL);
    uint32_t src = sel == 0 ? esp_rom_get_xtal_freq() / MHZ : src_mhz[sel - 1];
    return src / (REG_GET_FIELD(HP_SYS_CLKRST_PERI_CLK_CTRL00_REG, HP_SYS_CLKRST_REG_FLASH_CORE_CLK_DIV_NUM) + 1);
}

bool stub_target_flash_needs_attach(void)
{
    return !stub_target_cache_is_enabled();
}

void stub_target_flash_attach(uint32_t ishspi, bool legacy)
{
    /* If DOWNLOAD_MODE_XPD_ON eFuse is set, ROM powers on the flash chip
     * inside esp_rom_spiflash_attach.
     * The ECO7 power-on sequence cannot run twice, so skip it when PMU_DATE[1:0]
     * already force the rail on. ECO8 ROM powers the rail off first, so it can run again.
     * esp_rom_spiflash_boot_attach is not exported, its address is valid for ECO7 only. */
    if (_rom_eco_version == 7 && REG_GET_FIELD(EFUSE_RD_REPEAT_DATA1_REG, EFUSE_DOWNLOAD_MODE_XPD_ON) &&
        (REG_READ(PMU_DATE_REG) & 0x3) == 0x3) {
        esp_rom_spiflash_boot_attach(ishspi, legacy, true);
        return;
    }
    esp_rom_spiflash_attach(ishspi, legacy);
}

void stub_target_spi_wait_ready(void)
{
    while (REG_GET_FIELD(SPI1_MEM_C_CMD_REG, SPI1_MEM_C_MST_ST) ||
           REG_GET_FIELD(SPI1_MEM_C_CMD_REG, SPI1_MEM_C_SLV_ST)) {
        /* busy wait */
    }
}

uint32_t stub_target_get_max_supported_flash_size(void)
{
    /* ESP32-P4 supports up to 64MB with 4-byte addressing */
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

    stub_lib_cache_stop();

    /* Clear IDF's extra-dummy tuning on SPI0; otherwise it stacks on top of
     * the fixed dummy count programmed below and mis-times reads. */
    CLEAR_PERI_REG_MASK(SPI_MEM_C_TIMING_CALI_REG, SPI_MEM_C_TIMING_CALI_M);
    REG_SET_FIELD(SPI_MEM_C_TIMING_CALI_REG, SPI_MEM_C_EXTRA_DUMMY_CYCLELEN, 0);
    REG_SET_BIT(SPI_MEM_C_TIMING_CALI_REG, SPI_MEM_C_TIMING_CALI_UPDATE);
    rom_spiflash_legacy_data->dummy_len_plus[FLASH_SPI_NUM_INT] = 0;

    /* The P4 ROM has no esp_rom_opiflash_cache_mode_config(), so program SPI0
     * directly. Use FAST_READ_4B + 8 dummy; SLOWRD/0x13/0-dummy comes back
     * bit-shifted through SPI0 cache at typical flash clocks. */
    esp_rom_spi_set_op_mode(FLASH_SPI_NUM_INT, SPI_FLASH_FASTRD_MODE);
    esp_rom_spi_set_address_bit_len(FLASH_SPI_NUM_INT, ADDR32_CACHE_ADDR_BITLEN);
    REG_SET_BIT(SPI_MEM_C_CACHE_FCTRL_REG, SPI_MEM_C_CACHE_USR_ADDR_4BYTE);
    REG_SET_BIT(SPI_MEM_C_USER_REG, SPI_MEM_C_USR_DUMMY);
    REG_SET_FIELD(SPI_MEM_C_USER1_REG, SPI_MEM_C_USR_DUMMY_CYCLELEN, ADDR32_CACHE_DUMMY_BITLEN - 1);
    REG_SET_FIELD(SPI_MEM_C_USER2_REG, SPI_MEM_C_USR_COMMAND_VALUE, ADDR32_CACHE_CMD);
    REG_SET_FIELD(SPI_MEM_C_USER2_REG, SPI_MEM_C_USR_COMMAND_BITLEN, ADDR32_CACHE_CMD_BITLEN - 1);
    REG_SET_FIELD(SPI_MEM_C_DDR_REG, SPI_MEM_C_FMEM__VAR_DUMMY, 0);
    /* Keep the cache read dummy phase and WP/HOLD lines at high level. */
    REG_SET_BIT(SPI_MEM_C_CTRL_REG, SPI_MEM_C_FDUMMY_RIN | SPI_MEM_C_WP_REG | SPI_MEM_C_D_POL | SPI_MEM_C_Q_POL);

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

void stub_target_flash_init(void *state, stub_lib_flash_attach_policy_t attach_policy)
{
    (void)state;

    if (attach_policy == STUB_LIB_FLASH_ATTACH_ALWAYS || stub_target_flash_needs_attach()) {
        STUB_LOGD("Attach spi flash...\n");
        stub_target_flash_attach(0, false);
    }

    REG_SET_BIT(SPI_MEM_USER_REG(FLASH_SPI_NUM), SPI_MEM_USR_COMMAND);
}
