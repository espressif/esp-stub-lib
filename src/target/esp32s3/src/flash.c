/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/cache.h>
#include <esp-stub-lib/log.h>
#include <esp-stub-lib/rom_wrappers.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/flash.h>

#include <private/flash_commands.h>
#include <private/rom_flash.h>
#include <private/rom_flash_config.h>

#include <soc/spi_mem_compat.h>

extern void spi_cache_mode_switch(uint32_t modebit);
extern void spi_common_set_flash_cs_timing(void);
extern void esp_rom_opiflash_mode_reset(int spi_num);
extern esp_rom_spiflash_legacy_funcs_t *rom_spiflash_legacy_funcs;
extern esp_rom_spiflash_legacy_data_t *rom_spiflash_legacy_data;
extern uint32_t esp_rom_efuse_get_flash_gpio_info(void);
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

typedef struct {
    uint8_t addr_bit_len;
    uint8_t dummy_bit_len;
    uint16_t cmd;
    uint8_t cmd_bit_len;
    uint8_t var_dummy_en;
} esp_rom_opiflash_spi0rd_t;
extern void esp_rom_opiflash_cache_mode_config(spi_flash_mode_t mode, const esp_rom_opiflash_spi0rd_t *cache);

/* MXIC OPI mode-switch: WRCR2[0] = 0x01 (STR OPI) or 0x02 (DTR OPI). */
#define MXIC_CMD_WRCR2_SPI    0x72
#define MXIC_CR2_MODE_OPI_STR 0x01
#define MXIC_CR2_MODE_OPI_DTR 0x02

enum {
    SPI_USER_REG_ID = 0,
    SPI_CTRL_REG_ID,
    SPI_CTRL2_REG_ID,
    SPI_CLOCK_REG_ID,
    SPI_DDR_REG_ID,
    SPI_TIMING_CALI_REG_ID,
    SPI_REGS_NUM,
};

enum {
    SPI0_USER_REG_ID = 0,
    SPI0_USER1_REG_ID,
    SPI0_USER2_REG_ID,
    SPI0_CTRL_REG_ID,
    SPI0_CTRL2_REG_ID,
    SPI0_CACHE_FCTRL_REG_ID,
    SPI0_DDR_REG_ID,
    SPI0_TIMING_CALI_REG_ID,
    SPI0_CLOCK_REG_ID,
    SPI0_MISC_REG_ID,
    SPI0_MISO_DLEN_REG_ID,
    SPI0_MOSI_DLEN_REG_ID,
    SPI0_REGS_NUM,
};

typedef struct {
    uint32_t spi_regs[SPI_REGS_NUM];
    uint32_t spi0_regs[SPI0_REGS_NUM];
    uint8_t dummy_len_plus;
    uint8_t spi0_dummy_len_plus;
    uint8_t target_cr2;
    bool octal_active;
} stub_esp32s3_flash_state_t;

size_t stub_target_flash_state_size(void)
{
    return sizeof(stub_esp32s3_flash_state_t);
}

uint32_t stub_target_flash_get_spiconfig_efuse(void)
{
    return esp_rom_efuse_get_flash_gpio_info();
}

static void stub_target_flash_init_funcs(void)
{
    static esp_rom_spiflash_legacy_funcs_t funcs = {
        .se_addr_bit_len = 24,
        .be_addr_bit_len = 24,
        .pp_addr_bit_len = 24,
        .rd_addr_bit_len = 24,
        .read_sub_len = 16, // 32 - from IDF
        .write_sub_len = 32,
    };
    rom_spiflash_legacy_funcs = &funcs;
}

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
    while (REG_GET_FIELD(SPI_MEM_FSM_REG(FLASH_SPI_NUM), SPI_MEM_ST)) {
        /* busy wait */
    }
    /* There is no HW arbiter on internal SPI, so we need to wait for it to be ready */
    while ((REG_READ(SPI_MEM_FSM_REG(FLASH_SPI_NUM_INT)) & SPI_MEM_ST)) {
        /* busy wait */
    }
}

uint32_t stub_target_get_max_supported_flash_size(void)
{
    /* ESP32-S3 supports up to 1GB with 4-byte addressing */
    return GIB(1);
}

bool stub_target_flash_needs_attach(void)
{
    return (READ_PERI_REG(SPI_MEM_CACHE_FCTRL_REG(FLASH_SPI_NUM_INT)) & SPI_MEM_CACHE_FLASH_USR_CMD) == 0;
}

void stub_target_flash_state_save(void *state)
{
    if (!state) {
        return;
    }

    stub_esp32s3_flash_state_t *s = state;
    s->spi_regs[SPI_USER_REG_ID] = READ_PERI_REG(SPI_MEM_USER_REG(FLASH_SPI_NUM));
    s->spi_regs[SPI_CTRL_REG_ID] = READ_PERI_REG(SPI_MEM_CTRL_REG(FLASH_SPI_NUM));
    s->spi_regs[SPI_CTRL2_REG_ID] = READ_PERI_REG(SPI_MEM_CTRL2_REG(FLASH_SPI_NUM));
    s->spi_regs[SPI_CLOCK_REG_ID] = READ_PERI_REG(SPI_MEM_CLOCK_REG(FLASH_SPI_NUM));
    s->spi_regs[SPI_DDR_REG_ID] = READ_PERI_REG(SPI_MEM_DDR_REG(FLASH_SPI_NUM));
    s->spi_regs[SPI_TIMING_CALI_REG_ID] = READ_PERI_REG(SPI_MEM_TIMING_CALI_REG(FLASH_SPI_NUM));
    s->dummy_len_plus = rom_spiflash_legacy_data->dummy_len_plus[FLASH_SPI_NUM];
}

void stub_target_flash_state_restore(const void *state)
{
    if (!state)
        return;

    const stub_esp32s3_flash_state_t *s = state;
    WRITE_PERI_REG(SPI_MEM_CTRL_REG(FLASH_SPI_NUM), s->spi_regs[SPI_CTRL_REG_ID]);
    WRITE_PERI_REG(SPI_MEM_CTRL2_REG(FLASH_SPI_NUM), s->spi_regs[SPI_CTRL2_REG_ID]);
    WRITE_PERI_REG(SPI_MEM_CLOCK_REG(FLASH_SPI_NUM), s->spi_regs[SPI_CLOCK_REG_ID]);
    WRITE_PERI_REG(SPI_MEM_DDR_REG(FLASH_SPI_NUM), s->spi_regs[SPI_DDR_REG_ID]);
    WRITE_PERI_REG(SPI_MEM_USER_REG(FLASH_SPI_NUM), s->spi_regs[SPI_USER_REG_ID]);
    WRITE_PERI_REG(SPI_MEM_TIMING_CALI_REG(FLASH_SPI_NUM), s->spi_regs[SPI_TIMING_CALI_REG_ID]);
    rom_spiflash_legacy_data->dummy_len_plus[FLASH_SPI_NUM] = s->dummy_len_plus;
}

static void stub_target_spi_init(void)
{
    const uint32_t freqbits = 0x30103; /* precalculated for SPI_CLK_DIV(4) */

    /*
     * Trimmed version of ROM SPI_init(SLOWRD_MODE, 4).
     * We skip the module reset to avoid breaking communication with PSRAM.
     */
    REG_CLR_BIT(SPI_MEM_MISC_REG(FLASH_SPI_NUM_INT), SPI_MEM_CS0_DIS);
    REG_SET_BIT(SPI_MEM_MISC_REG(FLASH_SPI_NUM_INT), SPI_MEM_CS1_DIS);

    spi_common_set_flash_cs_timing();

    WRITE_PERI_REG(SPI_MEM_CLOCK_REG(FLASH_SPI_NUM), freqbits);
    WRITE_PERI_REG(SPI_MEM_CLOCK_REG(FLASH_SPI_NUM_INT), freqbits);

    WRITE_PERI_REG(SPI_MEM_CTRL_REG(FLASH_SPI_NUM), SPI_MEM_WP_REG | SPI_MEM_RESANDRES);
    WRITE_PERI_REG(SPI_MEM_CTRL_REG(FLASH_SPI_NUM_INT), SPI_MEM_WP_REG);

    /*
     * Clear SPI1's USER/USER1/USER2 to known-good defaults.
     * Later SPI1 callers reconfigure USER themselves.
     */
    WRITE_PERI_REG(SPI_MEM_USER_REG(FLASH_SPI_NUM), 0);
    WRITE_PERI_REG(SPI_MEM_USER1_REG(FLASH_SPI_NUM), 0);
    WRITE_PERI_REG(SPI_MEM_USER2_REG(FLASH_SPI_NUM), ((uint32_t)0x7) << SPI_MEM_USR_COMMAND_BITLEN_S);

    REG_SET_FIELD(SPI_MEM_MISO_DLEN_REG(FLASH_SPI_NUM_INT), SPI_MEM_USR_MISO_DBITLEN, 0xff);
    REG_SET_FIELD(SPI_MEM_MOSI_DLEN_REG(FLASH_SPI_NUM_INT), SPI_MEM_USR_MOSI_DBITLEN, 0xff);
    REG_SET_FIELD(SPI_MEM_USER2_REG(FLASH_SPI_NUM_INT), SPI_MEM_USR_COMMAND_BITLEN, 0x7);
    REG_SET_BIT(SPI_MEM_CACHE_FCTRL_REG(FLASH_SPI_NUM_INT), SPI_MEM_CACHE_REQ_EN);

    WRITE_PERI_REG(SPI_MEM_DDR_REG(FLASH_SPI_NUM_INT), 0);
    WRITE_PERI_REG(SPI_MEM_DDR_REG(FLASH_SPI_NUM), 0);
    spi_cache_mode_switch(0);
    REG_SET_BIT(SPI_MEM_CACHE_FCTRL_REG(FLASH_SPI_NUM_INT), SPI_MEM_CACHE_FLASH_USR_CMD);

    /* Clear IDF's extra-dummy tuning on SPI1; otherwise it shifts JEDEC ID
     * readback. SPI0 left alone (cache/PSRAM). Restored by flash_state_*(). */
    rom_spiflash_legacy_data->dummy_len_plus[FLASH_SPI_NUM] = 0;
    CLEAR_PERI_REG_MASK(SPI_MEM_TIMING_CALI_REG(FLASH_SPI_NUM), SPI_MEM_TIMING_CALI_M);
    REG_SET_FIELD(SPI_MEM_TIMING_CALI_REG(FLASH_SPI_NUM), SPI_MEM_EXTRA_DUMMY_CYCLELEN, 0);
}

static struct {
    bool applied;
    uint32_t saved_user;
    uint32_t saved_user1;
    uint32_t saved_user2;
    uint32_t saved_cache_fctrl;
    uint32_t saved_ctrl;
    uint32_t saved_timing_cali;
    uint8_t saved_dummy_len_plus;
} s_addr32_state;

static void octal_save_spi0_state(stub_esp32s3_flash_state_t *s)
{
    s->spi0_regs[SPI0_USER_REG_ID] = REG_READ(SPI_MEM_USER_REG(FLASH_SPI_NUM_INT));
    s->spi0_regs[SPI0_USER1_REG_ID] = REG_READ(SPI_MEM_USER1_REG(FLASH_SPI_NUM_INT));
    s->spi0_regs[SPI0_USER2_REG_ID] = REG_READ(SPI_MEM_USER2_REG(FLASH_SPI_NUM_INT));
    s->spi0_regs[SPI0_CTRL_REG_ID] = REG_READ(SPI_MEM_CTRL_REG(FLASH_SPI_NUM_INT));
    s->spi0_regs[SPI0_CTRL2_REG_ID] = REG_READ(SPI_MEM_CTRL2_REG(FLASH_SPI_NUM_INT));
    s->spi0_regs[SPI0_CACHE_FCTRL_REG_ID] = REG_READ(SPI_MEM_CACHE_FCTRL_REG(FLASH_SPI_NUM_INT));
    s->spi0_regs[SPI0_DDR_REG_ID] = REG_READ(SPI_MEM_DDR_REG(FLASH_SPI_NUM_INT));
    s->spi0_regs[SPI0_TIMING_CALI_REG_ID] = REG_READ(SPI_MEM_TIMING_CALI_REG(FLASH_SPI_NUM_INT));
    s->spi0_regs[SPI0_CLOCK_REG_ID] = REG_READ(SPI_MEM_CLOCK_REG(FLASH_SPI_NUM_INT));
    s->spi0_regs[SPI0_MISC_REG_ID] = REG_READ(SPI_MEM_MISC_REG(FLASH_SPI_NUM_INT));
    s->spi0_regs[SPI0_MISO_DLEN_REG_ID] = REG_READ(SPI_MEM_MISO_DLEN_REG(FLASH_SPI_NUM_INT));
    s->spi0_regs[SPI0_MOSI_DLEN_REG_ID] = REG_READ(SPI_MEM_MOSI_DLEN_REG(FLASH_SPI_NUM_INT));
    s->spi0_dummy_len_plus = rom_spiflash_legacy_data->dummy_len_plus[FLASH_SPI_NUM_INT];
    s->target_cr2 =
        (s->spi0_regs[SPI0_DDR_REG_ID] & SPI_MEM_SPI_FMEM_DDR_EN) ? MXIC_CR2_MODE_OPI_DTR : MXIC_CR2_MODE_OPI_STR;
    s->octal_active = true;
}

static void octal_restore_chip_mode(uint8_t target_cr2)
{
    esp_rom_opiflash_exec_cmd(FLASH_SPI_NUM,
                              SPI_FLASH_FASTRD_MODE,
                              CMD_WREN,
                              8,
                              0,
                              0,
                              0,
                              NULL,
                              0,
                              NULL,
                              0,
                              ESP_ROM_OPIFLASH_SEL_CS0,
                              false);

    stub_lib_delay_us(10);

    esp_rom_opiflash_exec_cmd(FLASH_SPI_NUM,
                              SPI_FLASH_FASTRD_MODE,
                              MXIC_CMD_WRCR2_SPI,
                              8,
                              0,
                              32,
                              0,
                              &target_cr2,
                              8,
                              NULL,
                              0,
                              ESP_ROM_OPIFLASH_SEL_CS0,
                              false);

    stub_lib_delay_us(40);
}

static void octal_restore_spi0_state(const stub_esp32s3_flash_state_t *s)
{
    REG_WRITE(SPI_MEM_CLOCK_REG(FLASH_SPI_NUM_INT), s->spi0_regs[SPI0_CLOCK_REG_ID]);
    REG_WRITE(SPI_MEM_USER_REG(FLASH_SPI_NUM_INT), s->spi0_regs[SPI0_USER_REG_ID]);
    REG_WRITE(SPI_MEM_USER1_REG(FLASH_SPI_NUM_INT), s->spi0_regs[SPI0_USER1_REG_ID]);
    REG_WRITE(SPI_MEM_USER2_REG(FLASH_SPI_NUM_INT), s->spi0_regs[SPI0_USER2_REG_ID]);
    REG_WRITE(SPI_MEM_CTRL_REG(FLASH_SPI_NUM_INT), s->spi0_regs[SPI0_CTRL_REG_ID]);
    REG_WRITE(SPI_MEM_CTRL2_REG(FLASH_SPI_NUM_INT), s->spi0_regs[SPI0_CTRL2_REG_ID]);
    REG_WRITE(SPI_MEM_CACHE_FCTRL_REG(FLASH_SPI_NUM_INT), s->spi0_regs[SPI0_CACHE_FCTRL_REG_ID]);
    REG_WRITE(SPI_MEM_DDR_REG(FLASH_SPI_NUM_INT), s->spi0_regs[SPI0_DDR_REG_ID]);
    REG_WRITE(SPI_MEM_TIMING_CALI_REG(FLASH_SPI_NUM_INT), s->spi0_regs[SPI0_TIMING_CALI_REG_ID]);
    REG_WRITE(SPI_MEM_MISC_REG(FLASH_SPI_NUM_INT), s->spi0_regs[SPI0_MISC_REG_ID]);
    REG_WRITE(SPI_MEM_MISO_DLEN_REG(FLASH_SPI_NUM_INT), s->spi0_regs[SPI0_MISO_DLEN_REG_ID]);
    REG_WRITE(SPI_MEM_MOSI_DLEN_REG(FLASH_SPI_NUM_INT), s->spi0_regs[SPI0_MOSI_DLEN_REG_ID]);
    rom_spiflash_legacy_data->dummy_len_plus[FLASH_SPI_NUM_INT] = s->spi0_dummy_len_plus;
}

static void enable_4byte_cache_mode(void)
{
    if (REG_GET_FIELD(SPI_MEM_USER1_REG(FLASH_SPI_NUM_INT), SPI_MEM_USR_ADDR_BITLEN) == 31)
        return;

    s_addr32_state.saved_user = REG_READ(SPI_MEM_USER_REG(FLASH_SPI_NUM_INT));
    s_addr32_state.saved_user1 = REG_READ(SPI_MEM_USER1_REG(FLASH_SPI_NUM_INT));
    s_addr32_state.saved_user2 = REG_READ(SPI_MEM_USER2_REG(FLASH_SPI_NUM_INT));
    s_addr32_state.saved_cache_fctrl = REG_READ(SPI_MEM_CACHE_FCTRL_REG(FLASH_SPI_NUM_INT));
    s_addr32_state.saved_ctrl = REG_READ(SPI_MEM_CTRL_REG(FLASH_SPI_NUM_INT));
    s_addr32_state.saved_timing_cali = REG_READ(SPI_MEM_TIMING_CALI_REG(FLASH_SPI_NUM_INT));
    s_addr32_state.saved_dummy_len_plus = rom_spiflash_legacy_data->dummy_len_plus[FLASH_SPI_NUM_INT];

    /* Clear IDF's extra-dummy tuning on SPI0; otherwise it stacks on top of
     * the dummy count we pass to cache_mode_config() and mis-times reads. */
    CLEAR_PERI_REG_MASK(SPI_MEM_TIMING_CALI_REG(FLASH_SPI_NUM_INT), SPI_MEM_TIMING_CALI_M);
    REG_SET_FIELD(SPI_MEM_TIMING_CALI_REG(FLASH_SPI_NUM_INT), SPI_MEM_EXTRA_DUMMY_CYCLELEN, 0);
    rom_spiflash_legacy_data->dummy_len_plus[FLASH_SPI_NUM_INT] = 0;

    /* Use FAST_READ_4B + 8 dummy; SLOWRD/0x13/0-dummy comes back bit-shifted
     * through SPI0 cache at typical flash clocks. Matches IDF. */
    const esp_rom_opiflash_spi0rd_t cache_rd = {
        .addr_bit_len = 32,
        .dummy_bit_len = 8,
        .cmd = CMD_FSTRD4B,
        .cmd_bit_len = 8,
        .var_dummy_en = 0,
    };

    STUB_LOGD("Switching SPI0 cache to 32-bit addr (cmd=0x%x, dummy=%u)\n", cache_rd.cmd, cache_rd.dummy_bit_len);

    stub_lib_cache_stop();
    esp_rom_opiflash_cache_mode_config(SPI_FLASH_FASTRD_MODE, &cache_rd);
    stub_lib_cache_start();

    s_addr32_state.applied = true;
}

static void disable_4byte_cache_mode(void)
{
    if (!s_addr32_state.applied)
        return;

    stub_lib_cache_stop();
    REG_WRITE(SPI_MEM_USER_REG(FLASH_SPI_NUM_INT), s_addr32_state.saved_user);
    REG_WRITE(SPI_MEM_USER1_REG(FLASH_SPI_NUM_INT), s_addr32_state.saved_user1);
    REG_WRITE(SPI_MEM_USER2_REG(FLASH_SPI_NUM_INT), s_addr32_state.saved_user2);
    REG_WRITE(SPI_MEM_CACHE_FCTRL_REG(FLASH_SPI_NUM_INT), s_addr32_state.saved_cache_fctrl);
    REG_WRITE(SPI_MEM_CTRL_REG(FLASH_SPI_NUM_INT), s_addr32_state.saved_ctrl);
    REG_WRITE(SPI_MEM_TIMING_CALI_REG(FLASH_SPI_NUM_INT), s_addr32_state.saved_timing_cali);
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
    bool octal_mode = ets_efuse_flash_octal_mode();
    bool needs_attach = (attach_policy == STUB_LIB_FLASH_ATTACH_ALWAYS) || stub_target_flash_needs_attach();
    stub_esp32s3_flash_state_t *s = state;

    if (s) {
        stub_target_flash_state_save(s);
        /* Buffer is allocated by the client and may be uninitialized. */
        s->octal_active = false;
        s->target_cr2 = 0;
    }

    if (needs_attach) {
        uint32_t spiconfig = stub_target_flash_get_spiconfig_efuse();
        stub_target_flash_attach(spiconfig, 0);
    } else {
        /* Partial init: full SPI module reset would break IDF's cache config
         * and PSRAM on the same bus. For octal, snapshot SPI0 first --
         * stub_target_spi_init() clobbers it, and deinit needs the original
         * to put the chip back into OPI. NULL state opts out of OPI restore. */
        if (octal_mode && s) {
            octal_save_spi0_state(s);
        }
        stub_target_spi_init();
        if (octal_mode) {
            esp_rom_opiflash_mode_reset(FLASH_SPI_NUM);
        }
    }

    REG_SET_BIT(SPI_MEM_USER_REG(FLASH_SPI_NUM), SPI_MEM_USR_COMMAND);

    if (octal_mode) {
        STUB_LOGD("octal mode is on (cr2_restore=0x%x)\n", s ? s->target_cr2 : 0);
        stub_target_flash_init_funcs();
    }
}

/* Restore chip + SPI0 to IDF's OPI mode; otherwise a chip-in-SPI /
 * controller-in-OPI mismatch crashes the app on the next cache fetch.
 * Opt-in via state buffer: NULL skips the restore. */
void stub_target_flash_deinit(const void *state)
{
    const stub_esp32s3_flash_state_t *s = state;

    if (s && s->octal_active) {
        stub_lib_cache_stop();
        octal_restore_chip_mode(s->target_cr2);
        octal_restore_spi0_state(s);
        stub_lib_cache_start();
    }
    stub_target_flash_state_restore(state);
}
