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
extern uint8_t g_rom_spiflash_dummy_len_plus[];

typedef struct {
    uint32_t spi_regs[SPI_REGS_NUM];
    uint8_t dummy_len_plus;
} stub_flash_state_t;

static stub_flash_state_t s_flash_state;

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

bool stub_target_flash_state_save(void **state)
{
    if ((READ_PERI_REG(SPI_CACHE_FCTRL_REG(0)) & SPI_CACHE_FLASH_USR_CMD) == 0) {
        /* Flash not initialized -- full attach, no state to save */
        return true;
    }

    /* Flash already initialized -- save SPI1 state, configure for stub */
    s_flash_state.spi_regs[SPI_USER_IDX] = READ_PERI_REG(SPI_USER_REG(1));
    s_flash_state.spi_regs[SPI_USER1_IDX] = READ_PERI_REG(SPI_USER1_REG(1));
    s_flash_state.spi_regs[SPI_USER2_IDX] = READ_PERI_REG(SPI_USER2_REG(1));
    s_flash_state.spi_regs[SPI_SLAVE_IDX] = READ_PERI_REG(SPI_SLAVE_REG(1));
    s_flash_state.spi_regs[SPI_CTRL_IDX] = READ_PERI_REG(SPI_CTRL_REG(1));
    s_flash_state.spi_regs[SPI_CLOCK_IDX] = READ_PERI_REG(SPI_CLOCK_REG(1));
    s_flash_state.dummy_len_plus = g_rom_spiflash_dummy_len_plus[1];

    /* Reset SPI1 user-mode registers to hardware defaults */
    WRITE_PERI_REG(SPI_USER_REG(1), 0x80000040UL);  /* SPI_USR_COMMAND | SPI_CK_I_EDGE (hw default) */
    WRITE_PERI_REG(SPI_USER1_REG(1), 0x5c000007UL); /* ADDR_BITLEN=23(3B), DUMMY_CYCLELEN=7(8clk) (hw default) */
    WRITE_PERI_REG(SPI_USER2_REG(1), 0x70000000UL); /* CMD_BITLEN=7(8bit), CMD_VALUE=0 (hw default) */
    WRITE_PERI_REG(SPI_SLAVE_REG(1), 0x00000200UL); /* master mode, TRANS_DONE int only (hw default) */
    /* Configure SPI1 for SLOWRD mode at 20MHz -- matches ROM SPI_init(SLOWRD, SPI_CLK_DIV=4) */
    WRITE_PERI_REG(SPI_CTRL_REG(1), 0x208000UL); /* SPI_WP_REG | SPI_RESANDRES, SLOWRD mode */
    WRITE_PERI_REG(SPI_CLOCK_REG(1), 0x3043UL);  /* N=3,H=1,L=3 -> APB/4 = 20MHz */
    g_rom_spiflash_dummy_len_plus[1] = 0;        /* no extra dummy (IOMUX routing) */

    *state = &s_flash_state;
    return false;
}

void stub_target_flash_state_restore(const void *state)
{
    const stub_flash_state_t *s = (const stub_flash_state_t *)state;

    if (!s) {
        return;
    }

    WRITE_PERI_REG(SPI_USER_REG(1), s->spi_regs[SPI_USER_IDX]);
    WRITE_PERI_REG(SPI_USER1_REG(1), s->spi_regs[SPI_USER1_IDX]);
    WRITE_PERI_REG(SPI_USER2_REG(1), s->spi_regs[SPI_USER2_IDX]);
    WRITE_PERI_REG(SPI_SLAVE_REG(1), s->spi_regs[SPI_SLAVE_IDX]);
    WRITE_PERI_REG(SPI_CTRL_REG(1), s->spi_regs[SPI_CTRL_IDX]);
    WRITE_PERI_REG(SPI_CLOCK_REG(1), s->spi_regs[SPI_CLOCK_IDX]);
    g_rom_spiflash_dummy_len_plus[1] = s->dummy_len_plus;
}
