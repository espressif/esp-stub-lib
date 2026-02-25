/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdbool.h>
#include <stdint.h>

#include <esp-stub-lib/log.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/flash.h>

#include <private/rom_flash.h>

#include <soc/spi_mem_compat.h>

extern esp_rom_spiflash_chip_t g_rom_flashchip;
extern uint32_t esp_rom_efuse_get_flash_gpio_info(void);
extern void esp_rom_spiflash_attach(uint32_t ishspi, bool legacy);

extern uint8_t g_rom_spiflash_dummy_len_plus[];

enum {
    SPI_USER_REG_ID = 0,
    SPI_USER1_REG_ID,
    SPI_USER2_REG_ID,
    SPI_SLAVE_REG_ID,
    SPI_CLOCK_REG_ID,
    SPI_CTRL_REG_ID,
    SPI_REGS_NUM,
};

typedef struct {
    uint32_t spi_regs[SPI_REGS_NUM];
    uint8_t dummy_len_plus;
} stub_esp32_flash_state_t;

static stub_esp32_flash_state_t s_flash_state;

uint32_t stub_target_flash_get_spiconfig_efuse(void)
{
    return esp_rom_efuse_get_flash_gpio_info();
}

esp_rom_spiflash_chip_t *stub_target_flash_get_config(void)
{
    return &g_rom_flashchip;
}

uint32_t stub_target_flash_get_flash_id(void)
{
    WRITE_PERI_REG(SPI_W0_REG(FLASH_SPI_NUM), 0); // clear register
    WRITE_PERI_REG(SPI_CMD_REG(FLASH_SPI_NUM), SPI_FLASH_RDID);
    while (READ_PERI_REG(SPI_CMD_REG(FLASH_SPI_NUM)) != 0) {
        /* busy wait */
    }
    return (REG_READ(SPI_W0_REG(FLASH_SPI_NUM)) & 0xffffff) >> 16;
}

void stub_target_spi_wait_ready(void)
{
    // No need to wait for SPI0 as there is a hardware arbiter between SPI0 and SPI1
    while (REG_GET_FIELD(SPI_EXT2_REG(FLASH_SPI_NUM), SPI_ST)) {
        /* busy wait */
    }
}

bool stub_target_flash_needs_attach(void)
{
    return (READ_PERI_REG(SPI_CACHE_FCTRL_REG(0)) & SPI_CACHE_FLASH_USR_CMD) == 0;
}

void stub_target_flash_state_save(void **state)
{
    if (!state) {
        return;
    }

    s_flash_state.spi_regs[SPI_USER_REG_ID] = READ_PERI_REG(SPI_USER_REG(FLASH_SPI_NUM));
    s_flash_state.spi_regs[SPI_USER1_REG_ID] = READ_PERI_REG(SPI_USER1_REG(FLASH_SPI_NUM));
    s_flash_state.spi_regs[SPI_USER2_REG_ID] = READ_PERI_REG(SPI_USER2_REG(FLASH_SPI_NUM));
    s_flash_state.spi_regs[SPI_SLAVE_REG_ID] = READ_PERI_REG(SPI_SLAVE_REG(FLASH_SPI_NUM));
    s_flash_state.spi_regs[SPI_CLOCK_REG_ID] = READ_PERI_REG(SPI_CLOCK_REG(1));
    s_flash_state.spi_regs[SPI_CTRL_REG_ID] = READ_PERI_REG(SPI_CTRL_REG(1));
    s_flash_state.dummy_len_plus = g_rom_spiflash_dummy_len_plus[1];

    *state = &s_flash_state;
}

void stub_target_flash_state_restore(const void *state)
{
    if (!state) {
        return;
    }

    const stub_esp32_flash_state_t *s = state;

    STUB_LOGD("SPI_USER_REG(1) was:0x%x, restored to:0x%x\n",
              READ_PERI_REG(SPI_USER_REG(1)),
              s->spi_regs[SPI_USER_REG_ID]);

    WRITE_PERI_REG(SPI_USER1_REG(1), s->spi_regs[SPI_USER1_REG_ID]);
    WRITE_PERI_REG(SPI_USER2_REG(1), s->spi_regs[SPI_USER2_REG_ID]);
    WRITE_PERI_REG(SPI_SLAVE_REG(1), s->spi_regs[SPI_SLAVE_REG_ID]);
    WRITE_PERI_REG(SPI_CLOCK_REG(1), s->spi_regs[SPI_CLOCK_REG_ID]);
    WRITE_PERI_REG(SPI_CTRL_REG(1), s->spi_regs[SPI_CTRL_REG_ID]);
    WRITE_PERI_REG(SPI_USER_REG(FLASH_SPI_NUM), s->spi_regs[SPI_USER_REG_ID]);
    g_rom_spiflash_dummy_len_plus[1] = s->dummy_len_plus;
}

/*
 * ROM esp_rom_spiflash_erase_area() calls esp_rom_spiflash_config_readmode(SLOWRD) first.
 * That clears QIO/fast-read bits on SPI_CTRL_REG(0) and SPI_CTRL_REG(1), reprograms
 * SPI0 user registers for slow XIP (cmd 0x03, etc.), and adjusts cache-related SPI
 * state — but it never restores the previous mapping after erase finishes. The CPU
 * then fetches from flash through SPI0/cache with the wrong protocol until something
 * reprograms those registers again, which breaks mmap reads inside the stub and
 * GDB memory reads over JTAG after the stub returns. Snapshot the clobbered
 * registers around the ROM call and write them back so XIP and the SPI1 erase path
 * match the configuration the firmware had before erase.
 */
int stub_target_rom_spiflash_erase_area(uint32_t addr, uint32_t size)
{
    uint32_t spi0_ctrl = READ_PERI_REG(SPI_CTRL_REG(0));
    uint32_t spi0_user = READ_PERI_REG(SPI_USER_REG(0));
    uint32_t spi0_user1 = READ_PERI_REG(SPI_USER1_REG(0));
    uint32_t spi0_user2 = READ_PERI_REG(SPI_USER2_REG(0));
    uint32_t spi1_ctrl = READ_PERI_REG(SPI_CTRL_REG(1));
    uint32_t spi0_cache_sctrl = READ_PERI_REG(SPI_CACHE_SCTRL_REG(0));

    int rom_res = esp_rom_spiflash_erase_area(addr, size);

    WRITE_PERI_REG(SPI_CTRL_REG(0), spi0_ctrl);
    WRITE_PERI_REG(SPI_USER_REG(0), spi0_user);
    WRITE_PERI_REG(SPI_USER1_REG(0), spi0_user1);
    WRITE_PERI_REG(SPI_USER2_REG(0), spi0_user2);
    WRITE_PERI_REG(SPI_CACHE_SCTRL_REG(0), spi0_cache_sctrl);
    WRITE_PERI_REG(SPI_CTRL_REG(1), spi1_ctrl);

    return rom_res;
}

static void stub_target_spi_init(void)
{
    /*
     * Trimmed version of ROM SPI_init(SLOWRD_MODE, 4).
     * We skip the module reset to avoid breaking communication with PSRAM.
     */
    WRITE_PERI_REG(SPI_CTRL_REG(1), SPI_WP_REG | SPI_RESANDRES);
    WRITE_PERI_REG(SPI_CLOCK_REG(1), 0x3043U); /* precalculated for SPI_CLK_DIV(4) */

    WRITE_PERI_REG(SPI_USER1_REG(1), 0);
    REG_SET_FIELD(SPI_USER1_REG(1), SPI_USR_ADDR_BITLEN, 23);
    REG_SET_FIELD(SPI_USER1_REG(1), SPI_USR_DUMMY_CYCLELEN, 7);

    g_rom_spiflash_dummy_len_plus[1] = 0;
}

void stub_target_flash_init(void **state, stub_lib_flash_attach_policy_t attach_policy)
{
    if (state) {
        stub_target_flash_state_save(state);
    }

    if (attach_policy == STUB_LIB_FLASH_ATTACH_ALWAYS || stub_target_flash_needs_attach()) {
        STUB_LOGD("Attach spi flash...\n");
        uint32_t spiconfig = stub_target_flash_get_spiconfig_efuse();
        esp_rom_spiflash_attach(spiconfig, 0);
    } else {
        stub_target_spi_init();
    }

    /*
     * Command phase is always set in download mode.
     * But in reset-run case, it seems to be not set.
     * So we need to set it here before sending any command.
     */
    REG_SET_BIT(SPI_USER_REG(1), SPI_USR_COMMAND);
}
