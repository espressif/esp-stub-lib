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

/* Save/restore SPI registers. Can be extended to more registers if needed. */
enum {
    SPI_USER_REG_ID = 0,
    SPI_REGS_NUM,
};

typedef struct {
    uint32_t spi_regs[SPI_REGS_NUM];
} stub_esp32_flash_state_t;

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
    if (!state) {
        return;
    }

    stub_esp32_flash_state_t *s = state;
    s->spi_regs[SPI_USER_REG_ID] = READ_PERI_REG(SPI_USER_REG(FLASH_SPI_NUM));
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

    WRITE_PERI_REG(SPI_USER_REG(FLASH_SPI_NUM), s->spi_regs[SPI_USER_REG_ID]);
}

/*
 * ESP32 ROM SPIEraseArea() switches flash read mode to slow read before erase.
 * That reprograms SPI0 read-mode bits and cache/XIP user registers, but ROM
 * does not restore the previous controller state before returning. Restore the
 * clobbered registers here so cache/XIP reads remain consistent after the ROM
 * erase call.
 */
int stub_target_rom_spiflash_erase_area(uint32_t addr, uint32_t size)
{
    uint32_t spi0_ctrl = READ_PERI_REG(SPI_CTRL_REG(0));
    uint32_t spi0_user = READ_PERI_REG(SPI_USER_REG(0));
    uint32_t spi0_user1 = READ_PERI_REG(SPI_USER1_REG(0));
    uint32_t spi0_user2 = READ_PERI_REG(SPI_USER2_REG(0));
    uint32_t spi1_ctrl = READ_PERI_REG(SPI_CTRL_REG(1));

    int rom_res = esp_rom_spiflash_erase_area(addr, size);

    WRITE_PERI_REG(SPI_CTRL_REG(0), spi0_ctrl);
    WRITE_PERI_REG(SPI_USER_REG(0), spi0_user);
    WRITE_PERI_REG(SPI_USER1_REG(0), spi0_user1);
    WRITE_PERI_REG(SPI_USER2_REG(0), spi0_user2);
    WRITE_PERI_REG(SPI_CTRL_REG(1), spi1_ctrl);

    return rom_res;
}

bool stub_target_flash_needs_attach(void)
{
    return (READ_PERI_REG(SPI_CACHE_FCTRL_REG(0)) & SPI_CACHE_FLASH_USR_CMD) == 0;
}

void stub_target_flash_init(void *state, stub_lib_flash_attach_policy_t attach_policy)
{
    (void)state;
    uint32_t spiconfig = stub_target_flash_get_spiconfig_efuse();

    if (attach_policy == STUB_LIB_FLASH_ATTACH_ALWAYS || stub_target_flash_needs_attach()) {
        stub_target_flash_attach(spiconfig, 0);
        /*
         * Command phase is always set in download mode.
         * But in reset-run case, it seems to be not set.
         * So we need to set it here before sending any command.
         */
        REG_SET_BIT(SPI_USER_REG(1), SPI_USR_COMMAND);
    }
}
