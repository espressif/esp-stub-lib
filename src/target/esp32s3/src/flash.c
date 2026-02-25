/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>

#include <esp-stub-lib/bit_utils.h>
#include <esp-stub-lib/err.h>
#include <esp-stub-lib/log.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/flash.h>

#include <private/rom_flash.h>
#include <private/rom_flash_config.h>

#include <soc/io_mux_reg.h>
#include <soc/spi_mem_compat.h>

#define SPI_INTERNAL 0

extern void spi_cache_mode_switch(uint32_t modebit);
extern void spi_common_set_flash_cs_timing(void);
extern void esp_rom_opiflash_mode_reset(int spi_num);
extern uint32_t ets_efuse_get_spiconfig(void);
extern esp_rom_spiflash_legacy_funcs_t *rom_spiflash_legacy_funcs;
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

enum {
    SPI_USER_REG_ID = 0,
    SPI_CTRL_REG_ID,
    SPI_CTRL2_REG_ID,
    SPI_CLOCK_REG_ID,
    SPI_DDR_REG_ID,
    SPI_REGS_NUM,
};

typedef struct {
    uint32_t spi_regs[SPI_REGS_NUM];
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
    /* There is no HW arbiter on SPI_INTERNAL, so we need to wait for it to be ready */
    while ((REG_READ(SPI_MEM_FSM_REG(SPI_INTERNAL)) & SPI_MEM_ST)) {
        /* busy wait */
    }
}

bool stub_target_flash_needs_attach(void)
{
    return (READ_PERI_REG(SPI_MEM_CACHE_FCTRL_REG(0)) & SPI_MEM_CACHE_FLASH_USR_CMD) == 0;
}

uint32_t stub_target_get_max_supported_flash_size(void)
{
    /* ESP32-S3 supports up to 1GB with 4-byte addressing */
    return GIB(1);
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
}

void stub_target_spi_init(void)
{
    const uint32_t freqbits = 0x30103; // precalculated frequency bits for SPI_CLK_DIV(4)

    // Modified version of SPI_init(SpiFlashRdMode mode, uint8_t freqdiv) from esp_rom project
    // We do no reset the SPI module in order not to break communication with the PSRAM
    // Settings are done for mode SPI_FLASH_SLOWRD_MODE (5) and freqdiv SPI_CLK_DIV (4)

    REG_CLR_BIT(SPI_MEM_MISC_REG(0), SPI_MEM_CS0_DIS);
    REG_SET_BIT(SPI_MEM_MISC_REG(0), SPI_MEM_CS1_DIS);

    spi_common_set_flash_cs_timing();

    WRITE_PERI_REG(SPI_MEM_CLOCK_REG(1), freqbits);
    WRITE_PERI_REG(SPI_MEM_CLOCK_REG(0), freqbits);

    WRITE_PERI_REG(SPI_MEM_CTRL_REG(1), SPI_MEM_WP_REG | SPI_MEM_RESANDRES);
    WRITE_PERI_REG(SPI_MEM_CTRL_REG(0), SPI_MEM_WP_REG);

    REG_SET_FIELD(SPI_MEM_MISO_DLEN_REG(0), SPI_MEM_USR_MISO_DBITLEN, 0xff);
    REG_SET_FIELD(SPI_MEM_MOSI_DLEN_REG(0), SPI_MEM_USR_MOSI_DBITLEN, 0xff);
    REG_SET_FIELD(SPI_MEM_USER2_REG(0), SPI_MEM_USR_COMMAND_BITLEN, 0x7);
    REG_SET_BIT(SPI_MEM_CACHE_FCTRL_REG(0), SPI_MEM_CACHE_REQ_EN);

    WRITE_PERI_REG(SPI_MEM_DDR_REG(0), 0);
    WRITE_PERI_REG(SPI_MEM_DDR_REG(1), 0);
    spi_cache_mode_switch(0);
    REG_SET_BIT(SPI_MEM_CACHE_FCTRL_REG(0), SPI_MEM_CACHE_FLASH_USR_CMD);
}

// TODO: should we take care cache related settings from here?
int stub_target_flash_update_config(uint32_t flash_id,
                                    uint32_t flash_size,
                                    uint32_t block_size,
                                    uint32_t sector_size,
                                    uint32_t page_size,
                                    uint32_t status_mask)
{
    int res = esp_rom_spiflash_config_param(flash_id, flash_size, block_size, sector_size, page_size, status_mask);
    if (res != ESP_ROM_SPIFLASH_RESULT_OK) {
        STUB_LOGE("Failed to update flash config: %d\n", res);
        return STUB_LIB_FAIL;
    }

    /* spi_cache_mode_switch(0) configures SPI0 with 24-bit addressing (cmd 0x03).
     * For flash >= 16MB, override to 32-bit addressing (cmd 0x13) so that
     * cache/MMU reads can access addresses above 0x1000000. */
    if (flash_size > MIB(16)) {
        STUB_LOGD("Large flash (%d MB), enabling 4-byte cache addressing\n", BYTES_TO_MIB(flash_size));
        REG_SET_FIELD(SPI_MEM_USER1_REG(0), SPI_MEM_USR_ADDR_BITLEN, 31);
        REG_SET_FIELD(SPI_MEM_USER2_REG(0), SPI_MEM_USR_COMMAND_VALUE, 0x13);
        REG_SET_BIT(SPI_MEM_CACHE_FCTRL_REG(0), SPI_MEM_CACHE_USR_CMD_4BYTE);
    }

    return STUB_LIB_OK;
}

void stub_target_flash_init(void *state, stub_lib_flash_attach_policy_t attach_policy)
{
    bool octal_mode = ets_efuse_flash_octal_mode();

    if (state) {
        stub_target_flash_state_save(state);
    }

    if (attach_policy == STUB_LIB_FLASH_ATTACH_ALWAYS || stub_target_flash_needs_attach()) {
        STUB_LOGD("Attach spi flash...\n");
        uint32_t spiconfig = stub_target_flash_get_spiconfig_efuse();
        stub_target_flash_attach(spiconfig, 0);
    } else {
        stub_target_spi_init();
        if (octal_mode) {
            esp_rom_opiflash_mode_reset(FLASH_SPI_NUM);
        }
    }

    REG_SET_BIT(SPI_MEM_USER_REG(1), SPI_MEM_USR_COMMAND);

    if (octal_mode) {
        STUB_LOGD("octal mode is on\n");
        stub_target_flash_init_funcs();
    }
}
