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

/* Save/restore SPI registers. Can be extended to more registers if needed. */
enum {
    SPI_USER_REG_ID = 0,
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

void stub_target_flash_state_save(void **state)
{
    if (!state) {
        return;
    }

    s_flash_state.spi_regs[SPI_USER_REG_ID] = READ_PERI_REG(SPI_USER_REG(FLASH_SPI_NUM));
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

    WRITE_PERI_REG(SPI_CLOCK_REG(1), s->spi_regs[SPI_CLOCK_REG_ID]);
    WRITE_PERI_REG(SPI_CTRL_REG(1), s->spi_regs[SPI_CTRL_REG_ID]);
    WRITE_PERI_REG(SPI_USER_REG(FLASH_SPI_NUM), s->spi_regs[SPI_USER_REG_ID]);
    g_rom_spiflash_dummy_len_plus[1] = s->dummy_len_plus;
}

#define ESP32_STUB_FLASH_STATE_SPI_USER_REG_VAL  0x80000040UL
#define ESP32_STUB_FLASH_STATE_SPI_USER1_REG_VAL 0x5c000007UL
#define ESP32_STUB_FLASH_STATE_SPI_USER2_REG_VAL 0x70000000UL
#define ESP32_STUB_FLASH_STATE_SPI_SLAVE_REG_VAL 0x00000200UL
#define ESP32_STUB_FLASH_STATE_SPI_CTRL_REG_VAL  0x208000UL
#define ESP32_STUB_FLASH_STATE_SPI_CLOCK_REG_VAL 0x3043UL
void stub_target_flash_init(void **state)
{
    bool attach = true;

    if (state) {
        stub_target_flash_state_save(state);
        if (READ_PERI_REG(SPI_CACHE_FCTRL_REG(0)) & SPI_CACHE_FLASH_USR_CMD) {
            attach = false;
        }
    }

    // WRITE_PERI_REG(SPI_USER_REG(1), ESP32_STUB_FLASH_STATE_SPI_USER_REG_VAL);
    // WRITE_PERI_REG(SPI_USER1_REG(1), ESP32_STUB_FLASH_STATE_SPI_USER1_REG_VAL); //*** */
    // WRITE_PERI_REG(SPI_USER2_REG(1), ESP32_STUB_FLASH_STATE_SPI_USER2_REG_VAL);
    // WRITE_PERI_REG(SPI_SLAVE_REG(1), ESP32_STUB_FLASH_STATE_SPI_SLAVE_REG_VAL);

    if (attach) {
        STUB_LOGD("Attach spi flash...\n");
        uint32_t spiconfig = stub_target_flash_get_spiconfig_efuse();
        stub_target_flash_attach(spiconfig, 0);
    } else {
        WRITE_PERI_REG(SPI_CTRL_REG(1), 0x208000);
        WRITE_PERI_REG(SPI_CLOCK_REG(1), 0x3043);
        WRITE_PERI_REG(SPI_USER1_REG(1), ESP32_STUB_FLASH_STATE_SPI_USER1_REG_VAL);
        g_rom_spiflash_dummy_len_plus[1] = 0;
    }

    /*
     * Command phase is always set in download mode.
     * But in reset-run case, it seems to be not set.
     * So we need to set it here before sending any command.
     */
    REG_SET_BIT(SPI_USER_REG(1), SPI_USR_COMMAND);
}
