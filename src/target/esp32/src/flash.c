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

size_t stub_target_flash_state_size(void)
{
    return sizeof(stub_esp32_flash_state_t);
}

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

void stub_target_flash_state_save(void *state)
{
    if (!state)
        return;

    stub_esp32_flash_state_t *s = state;
    s->spi_regs[SPI_USER_REG_ID] = READ_PERI_REG(SPI_USER_REG(FLASH_SPI_NUM));
    s->spi_regs[SPI_USER1_REG_ID] = READ_PERI_REG(SPI_USER1_REG(FLASH_SPI_NUM));
    s->spi_regs[SPI_USER2_REG_ID] = READ_PERI_REG(SPI_USER2_REG(FLASH_SPI_NUM));
    s->spi_regs[SPI_SLAVE_REG_ID] = READ_PERI_REG(SPI_SLAVE_REG(FLASH_SPI_NUM));
    s->spi_regs[SPI_CLOCK_REG_ID] = READ_PERI_REG(SPI_CLOCK_REG(FLASH_SPI_NUM));
    s->spi_regs[SPI_CTRL_REG_ID] = READ_PERI_REG(SPI_CTRL_REG(FLASH_SPI_NUM));
    s->dummy_len_plus = g_rom_spiflash_dummy_len_plus[FLASH_SPI_NUM];
}

void stub_target_flash_state_restore(const void *state)
{
    if (!state) {
        return;
    }

    const stub_esp32_flash_state_t *s = state;

    WRITE_PERI_REG(SPI_USER1_REG(FLASH_SPI_NUM), s->spi_regs[SPI_USER1_REG_ID]);
    WRITE_PERI_REG(SPI_USER2_REG(FLASH_SPI_NUM), s->spi_regs[SPI_USER2_REG_ID]);
    WRITE_PERI_REG(SPI_SLAVE_REG(FLASH_SPI_NUM), s->spi_regs[SPI_SLAVE_REG_ID]);
    WRITE_PERI_REG(SPI_CLOCK_REG(FLASH_SPI_NUM), s->spi_regs[SPI_CLOCK_REG_ID]);
    WRITE_PERI_REG(SPI_CTRL_REG(FLASH_SPI_NUM), s->spi_regs[SPI_CTRL_REG_ID]);
    WRITE_PERI_REG(SPI_USER_REG(FLASH_SPI_NUM), s->spi_regs[SPI_USER_REG_ID]);
    g_rom_spiflash_dummy_len_plus[FLASH_SPI_NUM] = s->dummy_len_plus;
}

/*
 * ESP32 ROM SPIEraseArea() starts by forcing flash read mode to slow read via
 * SPIReadModeCnfig(SPI_FLASH_SLOWRD_MODE, true).
 *
 * That ROM path does not restore the previous state before returning. It:
 * - clears flash read-mode bits in PERIPHS_SPI_FLASH_CTRL / SPI_CTRL(0)
 * - reprograms SPI0 cache/XIP user registers via spi_cache_mode_switch()
 *   (SPI_USER(0), SPI_USER1(0), SPI_USER2(0), related cache-facing state)
 * - with legacy=true, may also disable the flash chip QE bit when leaving
 *   QIO/QOUT mode
 *
 * Restoring the controller registers here keeps cache/XIP reads consistent
 * after the ROM erase call. Note that this only restores controller state;
 * if QE preservation is required for the previous flash mode, that must be
 * handled separately.
 */
int stub_target_rom_spiflash_erase_area(uint32_t addr, uint32_t size)
{
    uint32_t spi0_ctrl = READ_PERI_REG(SPI_CTRL_REG(FLASH_SPI_NUM_INT));
    uint32_t spi0_user = READ_PERI_REG(SPI_USER_REG(FLASH_SPI_NUM_INT));
    uint32_t spi0_user1 = READ_PERI_REG(SPI_USER1_REG(FLASH_SPI_NUM_INT));
    uint32_t spi0_user2 = READ_PERI_REG(SPI_USER2_REG(FLASH_SPI_NUM_INT));
    uint32_t spi1_ctrl = READ_PERI_REG(SPI_CTRL_REG(FLASH_SPI_NUM));

    int rom_res = esp_rom_spiflash_erase_area(addr, size);

    WRITE_PERI_REG(SPI_CTRL_REG(FLASH_SPI_NUM_INT), spi0_ctrl);
    WRITE_PERI_REG(SPI_USER_REG(FLASH_SPI_NUM_INT), spi0_user);
    WRITE_PERI_REG(SPI_USER1_REG(FLASH_SPI_NUM_INT), spi0_user1);
    WRITE_PERI_REG(SPI_USER2_REG(FLASH_SPI_NUM_INT), spi0_user2);
    WRITE_PERI_REG(SPI_CTRL_REG(FLASH_SPI_NUM), spi1_ctrl);

    return rom_res;
}

bool stub_target_flash_needs_attach(void)
{
    return (READ_PERI_REG(SPI_CACHE_FCTRL_REG(FLASH_SPI_NUM_INT)) & SPI_CACHE_FLASH_USR_CMD) == 0;
}

static void stub_target_spi_init(void)
{
    /*
     * Trimmed version of ROM SPI_init(SLOWRD_MODE, 4).
     * We skip the module reset to avoid breaking communication with PSRAM.
     */
    WRITE_PERI_REG(SPI_CTRL_REG(FLASH_SPI_NUM), SPI_WP_REG | SPI_RESANDRES);
    WRITE_PERI_REG(SPI_CLOCK_REG(FLASH_SPI_NUM), 0x3043U); /* precalculated for SPI_CLK_DIV(4) */

    WRITE_PERI_REG(SPI_USER1_REG(FLASH_SPI_NUM), 0);
    REG_SET_FIELD(SPI_USER1_REG(FLASH_SPI_NUM), SPI_USR_ADDR_BITLEN, 23);
    REG_SET_FIELD(SPI_USER1_REG(FLASH_SPI_NUM), SPI_USR_DUMMY_CYCLELEN, 7);

    g_rom_spiflash_dummy_len_plus[FLASH_SPI_NUM] = 0;
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

    /*
     * Command phase is always set in download mode.
     * But in reset-run case, it seems to be not set.
     * So we need to set it here before sending any command.
     */
    REG_SET_BIT(SPI_USER_REG(FLASH_SPI_NUM), SPI_USR_COMMAND);
}
