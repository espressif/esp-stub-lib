/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>

#include <esp-stub-lib/soc_utils.h>

#include <target/flash.h>

#include <private/rom_flash.h>

#include <soc/spi_mem_compat.h>

extern esp_rom_spiflash_chip_t g_rom_flashchip;
extern uint32_t esp_rom_efuse_get_flash_gpio_info(void);
extern void spi_cache_mode_switch(uint32_t modebit);
extern void spi_common_set_flash_cs_timing(void);

enum {
    SPI_USER_REG_ID = 0,
    SPI_CLOCK_REG_ID,
    SPI_CTRL_REG_ID,
    SPI_REGS_NUM,
};

typedef struct {
    uint32_t spi_regs[SPI_REGS_NUM];
} stub_esp32s2_flash_state_t;

size_t stub_target_flash_state_size(void)
{
    return sizeof(stub_esp32s2_flash_state_t);
}

uint32_t stub_target_flash_get_spiconfig_efuse(void)
{
    return esp_rom_efuse_get_flash_gpio_info();
}

esp_rom_spiflash_chip_t *stub_target_flash_get_config(void)
{
    return &g_rom_flashchip;
}

void stub_target_spi_wait_ready(void)
{
    while ((REG_READ(SPI_MEM_FSM_REG(FLASH_SPI_NUM)) & SPI_MEM_ST)) {
        /* busy wait */
    }

    /* There is no HW arbiter on internal SPI, so we need to wait for it to be ready */
    while ((REG_READ(SPI_MEM_FSM_REG(FLASH_SPI_NUM_INT)) & SPI_MEM_ST)) {
        /* busy wait */
    }
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

    stub_esp32s2_flash_state_t *s = state;
    s->spi_regs[SPI_USER_REG_ID] = READ_PERI_REG(SPI_MEM_USER_REG(FLASH_SPI_NUM));
    s->spi_regs[SPI_CLOCK_REG_ID] = READ_PERI_REG(SPI_MEM_CLOCK_REG(FLASH_SPI_NUM));
    s->spi_regs[SPI_CTRL_REG_ID] = READ_PERI_REG(SPI_MEM_CTRL_REG(FLASH_SPI_NUM));
}

void stub_target_flash_state_restore(const void *state)
{
    if (!state) {
        return;
    }

    const stub_esp32s2_flash_state_t *s = state;

    WRITE_PERI_REG(SPI_MEM_CLOCK_REG(FLASH_SPI_NUM), s->spi_regs[SPI_CLOCK_REG_ID]);
    WRITE_PERI_REG(SPI_MEM_CTRL_REG(FLASH_SPI_NUM), s->spi_regs[SPI_CTRL_REG_ID]);
    WRITE_PERI_REG(SPI_MEM_USER_REG(FLASH_SPI_NUM), s->spi_regs[SPI_USER_REG_ID]);
}

static void stub_target_spi_init(void)
{
    const uint32_t freqbits = 0x30103; /* precalculated for SPI_CLK_DIV(4) */

    /* Trimmed version of ROM SPI_init(SLOWRD_MODE, 4).
     * We skip the module reset to avoid breaking communication with PSRAM. */
    REG_CLR_BIT(SPI_MEM_MISC_REG(FLASH_SPI_NUM_INT), SPI_MEM_CS0_DIS);
    REG_SET_BIT(SPI_MEM_MISC_REG(FLASH_SPI_NUM_INT), SPI_MEM_CS1_DIS);

    spi_common_set_flash_cs_timing();

    WRITE_PERI_REG(SPI_MEM_CLOCK_REG(FLASH_SPI_NUM), freqbits);
    WRITE_PERI_REG(SPI_MEM_CLOCK_REG(FLASH_SPI_NUM_INT), freqbits);

    WRITE_PERI_REG(SPI_MEM_CTRL_REG(FLASH_SPI_NUM), SPI_MEM_WP_REG | SPI_MEM_RESANDRES);
    WRITE_PERI_REG(SPI_MEM_CTRL_REG(FLASH_SPI_NUM_INT), SPI_MEM_WP_REG);

    REG_SET_FIELD(SPI_MEM_MISO_DLEN_REG(FLASH_SPI_NUM_INT), SPI_MEM_USR_MISO_DBITLEN, 0xff);
    REG_SET_FIELD(SPI_MEM_MOSI_DLEN_REG(FLASH_SPI_NUM_INT), SPI_MEM_USR_MOSI_DBITLEN, 0xff);
    REG_SET_FIELD(SPI_MEM_USER2_REG(FLASH_SPI_NUM_INT), SPI_MEM_USR_COMMAND_BITLEN, 0x7);
    REG_SET_BIT(SPI_MEM_CACHE_FCTRL_REG(FLASH_SPI_NUM_INT), SPI_MEM_CACHE_REQ_EN);

    WRITE_PERI_REG(SPI_MEM_DDR_REG(FLASH_SPI_NUM_INT), 0);
    WRITE_PERI_REG(SPI_MEM_DDR_REG(FLASH_SPI_NUM), 0);
    spi_cache_mode_switch(0);
    REG_SET_BIT(SPI_MEM_CACHE_FCTRL_REG(FLASH_SPI_NUM_INT), SPI_MEM_CACHE_FLASH_USR_CMD);
}

void stub_target_flash_init(void *state, stub_lib_flash_attach_policy_t attach_policy)
{
    if (state) {
        stub_target_flash_state_save(state);
    }

    if (attach_policy == STUB_LIB_FLASH_ATTACH_ALWAYS || stub_target_flash_needs_attach()) {
        uint32_t spiconfig = stub_target_flash_get_spiconfig_efuse();
        stub_target_flash_attach(spiconfig, 0);
    } else {
        stub_target_spi_init();
    }

    REG_SET_BIT(SPI_MEM_USER_REG(FLASH_SPI_NUM), SPI_MEM_USR_COMMAND);
}
