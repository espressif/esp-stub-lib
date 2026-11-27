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

#include <soc/spi_mem_compat.h>

/* ECO version from ROM - used to route to correct ROM functions */
extern uint32_t _rom_eco_version;
extern void esp_rom_spi_set_address_bit_len(int spi_num, int addr_bits);

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

#define ADDR24_CACHE_ADDR_BITLEN  24
#define ADDR32_CACHE_ADDR_BITLEN  32
#define ADDR32_CACHE_CMD_BITLEN   8
#define ADDR32_CACHE_MODE_BITLEN  8
#define ADDR32_CACHE_FASTRD_DUMMY 8

#define CMD_FASTRD_QIO_4B         0xEC
#define CMD_FASTRD_DIO_4B         0xBC
#define CMD_FASTRD_QUAD_4B        0x6C
#define CMD_FASTRD_DUAL_4B        0x3C

static struct {
    bool applied;
    uint32_t saved_user;
    uint32_t saved_user1;
    uint32_t saved_user2;
    uint32_t saved_ctrl;
    uint32_t saved_ddr;
    uint32_t saved_cache_fctrl;
    uint32_t saved_rd_status;
} s_addr32_state;

static void enable_4byte_cache_mode(void)
{
    /* If SPI0 cache already sends 4-byte addresses, leave it alone.
     * USR_ADDR_BITLEN can't tell: 24-bit QIO folds the 8 mode bits into the
     * address phase, which makes it 32 bits long too. */
    if (REG_GET_BIT(SPI_MEM_CACHE_FCTRL_REG(FLASH_SPI_NUM_INT), SPI_MEM_CACHE_USR_ADDR_4BYTE)) {
        return;
    }

    uint32_t ctrl = REG_READ(SPI_MEM_CTRL_REG(FLASH_SPI_NUM_INT));
    uint32_t addr_bits = REG_GET_FIELD(SPI_MEM_USER1_REG(FLASH_SPI_NUM_INT), SPI_MEM_USR_ADDR_BITLEN) + 1;
    uint32_t dummy_cycles = 0;
    uint32_t mode_cycles = 0;
    uint32_t addr_lines = 1;
    uint32_t cmd;

    if (REG_GET_BIT(SPI_MEM_USER_REG(FLASH_SPI_NUM_INT), SPI_MEM_USR_DUMMY)) {
        dummy_cycles = REG_GET_FIELD(SPI_MEM_USER1_REG(FLASH_SPI_NUM_INT), SPI_MEM_USR_DUMMY_CYCLELEN) + 1;
    }

    /* Keep the line mode and IDF's timing tuning, as PSRAM shares SPI0. */
    if (ctrl & SPI_MEM_FREAD_QIO) {
        cmd = CMD_FASTRD_QIO_4B;
        addr_lines = 4;
    } else if (ctrl & SPI_MEM_FREAD_DIO) {
        cmd = CMD_FASTRD_DIO_4B;
        addr_lines = 2;
    } else if (ctrl & SPI_MEM_FREAD_QUAD) {
        cmd = CMD_FASTRD_QUAD_4B;
    } else if (ctrl & SPI_MEM_FREAD_DUAL) {
        cmd = CMD_FASTRD_DUAL_4B;
    } else {
        /* The 4-byte slow read (0x13) is speed limited, so use fast read. */
        cmd = CMD_FSTRD4B;
        if (!(ctrl & SPI_MEM_FASTRD_MODE)) {
            dummy_cycles = ADDR32_CACHE_FASTRD_DUMMY;
        }
    }

    /* The ROM appends the DIO/QIO mode bits to a 24-bit address, but the
     * address phase holds at most 32 bits. Like ESP-IDF, send them as a
     * separate 0x00 mode byte, which keeps the flash out of continuous read
     * mode, and keep the cycle count between address and data. */
    if (addr_lines > 1) {
        if (addr_bits > ADDR24_CACHE_ADDR_BITLEN) {
            dummy_cycles += (addr_bits - ADDR24_CACHE_ADDR_BITLEN) / addr_lines;
        }
        mode_cycles = ADDR32_CACHE_MODE_BITLEN / addr_lines;
        dummy_cycles = dummy_cycles > mode_cycles ? dummy_cycles - mode_cycles : 0;
    }

    s_addr32_state.saved_user = REG_READ(SPI_MEM_USER_REG(FLASH_SPI_NUM_INT));
    s_addr32_state.saved_user1 = REG_READ(SPI_MEM_USER1_REG(FLASH_SPI_NUM_INT));
    s_addr32_state.saved_user2 = REG_READ(SPI_MEM_USER2_REG(FLASH_SPI_NUM_INT));
    s_addr32_state.saved_ctrl = ctrl;
    s_addr32_state.saved_ddr = REG_READ(SPI_MEM_DDR_REG(FLASH_SPI_NUM_INT));
    s_addr32_state.saved_cache_fctrl = REG_READ(SPI_MEM_CACHE_FCTRL_REG(FLASH_SPI_NUM_INT));
    s_addr32_state.saved_rd_status = REG_READ(SPI_MEM_RD_STATUS_REG(FLASH_SPI_NUM_INT));

    STUB_LOGD("Switching SPI0 cache to 32-bit addr (cmd=0x%x, mode=%u, dummy=%u)\n", cmd, mode_cycles, dummy_cycles);

    stub_lib_cache_stop();

    /* The C61 ROM has no esp_rom_opiflash_cache_mode_config(), so program SPI0
     * directly. esp_rom_spi_set_address_bit_len() also sets CACHE_USR_ADDR_4BYTE. */
    REG_SET_BIT(SPI_MEM_CTRL_REG(FLASH_SPI_NUM_INT), SPI_MEM_FASTRD_MODE);
    esp_rom_spi_set_address_bit_len(FLASH_SPI_NUM_INT, ADDR32_CACHE_ADDR_BITLEN);
    if (dummy_cycles) {
        REG_SET_BIT(SPI_MEM_USER_REG(FLASH_SPI_NUM_INT), SPI_MEM_USR_DUMMY);
        REG_SET_FIELD(SPI_MEM_USER1_REG(FLASH_SPI_NUM_INT), SPI_MEM_USR_DUMMY_CYCLELEN, dummy_cycles - 1);
    } else {
        REG_CLR_BIT(SPI_MEM_USER_REG(FLASH_SPI_NUM_INT), SPI_MEM_USR_DUMMY);
    }
    REG_SET_FIELD(SPI_MEM_USER2_REG(FLASH_SPI_NUM_INT), SPI_MEM_USR_COMMAND_VALUE, cmd);
    REG_SET_FIELD(SPI_MEM_USER2_REG(FLASH_SPI_NUM_INT), SPI_MEM_USR_COMMAND_BITLEN, ADDR32_CACHE_CMD_BITLEN - 1);
    REG_SET_FIELD(SPI_MEM_DDR_REG(FLASH_SPI_NUM_INT), SPI_FMEM_VAR_DUMMY, 0);
    if (mode_cycles) {
        REG_SET_FIELD(SPI_MEM_RD_STATUS_REG(FLASH_SPI_NUM_INT), SPI_MEM_WB_MODE, 0x00);
        REG_SET_FIELD(SPI_MEM_RD_STATUS_REG(FLASH_SPI_NUM_INT), SPI_MEM_WB_MODE_BITLEN, ADDR32_CACHE_MODE_BITLEN - 1);
        REG_SET_BIT(SPI_MEM_RD_STATUS_REG(FLASH_SPI_NUM_INT), SPI_MEM_WB_MODE_EN);
    }

    stub_lib_cache_start();

    /* Drop lines fetched with the previous read configuration. */
    stub_lib_cache_invalidate_all();

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
    REG_WRITE(SPI_MEM_RD_STATUS_REG(FLASH_SPI_NUM_INT), s_addr32_state.saved_rd_status);
    stub_lib_cache_start();

    /* Drop lines fetched with the 4-byte read configuration. */
    stub_lib_cache_invalidate_all();

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
