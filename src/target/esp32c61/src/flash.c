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

#include <soc/spi_mem_compat.h>

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

enum { SPI_CTRL_REG_ID, SPI_REGS_MAX };

typedef struct {
    uint32_t spi_regs[SPI_REGS_MAX];
} stub_esp32c61_flash_state_t;

static stub_esp32c61_flash_state_t s_flash_state;

void stub_target_flash_state_save(void **state)
{
    if (!state) {
        return;
    }

    s_flash_state.spi_regs[SPI_CTRL_REG_ID] = READ_PERI_REG(SPI_MEM_CTRL_REG(FLASH_SPI_NUM));

    *state = &s_flash_state;
}

void stub_target_flash_state_restore(const void *state)
{
    if (!state) {
        return;
    }

    const stub_esp32c61_flash_state_t *s = state;

    WRITE_PERI_REG(SPI_MEM_CTRL_REG(FLASH_SPI_NUM), s->spi_regs[SPI_CTRL_REG_ID]);
}

#include <soc/ext_mem_defs.h>
void stub_target_flash_init(void **state)
{
    bool attach = true;

    if (state) {
        stub_target_flash_state_save(state);
        if (READ_PERI_REG(SPI_MEM_CACHE_FCTRL_REG(0)) & SPI_MEM_CACHE_FLASH_USR_CMD) {
            // OpenOCD can not halt the target at the reset vector. So this bit will be read as set always
        }

        for (uint32_t i = 0; i < SOC_MMU_ENTRY_NUM; i++) {
            REG_WRITE(SPI_MEM_MMU_ITEM_INDEX_REG(0), i);
            uint32_t content = REG_READ(SPI_MEM_MMU_ITEM_CONTENT_REG(0));
            if (content & SOC_MMU_VALID) {
                attach = false;
                break;
            }
        }
    }

    if (attach) {
        STUB_LOGD("Attach spi flash...\n");
        stub_target_flash_attach(0, 0);
    } else {
        /*
         * When we skip esp_rom_spiflash_attach(), we must reproduce the minimum SPI1 setup.
         * SYNC_RESET clears a stuck MSPI state machine.
         * Clearing SPI_MEM_FLASH_SUS drops any suspend status that could block
         * a new erase or program sequence.
         * Writing SPI_MEM_CTRL with WP and RESANDRES drives /WP high to release hardware write protect.
         */
        REG_SET_BIT(SPI_MEM_CTRL2_REG(FLASH_SPI_NUM), SPI_MEM_SYNC_RESET);
        CLEAR_PERI_REG_MASK(SPI_MEM_SUS_STATUS_REG(FLASH_SPI_NUM), SPI_MEM_FLASH_SUS_M);
        WRITE_PERI_REG(SPI_MEM_CTRL_REG(FLASH_SPI_NUM), SPI_MEM_WP_REG | SPI_MEM_RESANDRES);
    }

    REG_SET_BIT(SPI_MEM_USER_REG(1), SPI_MEM_USR_COMMAND);
}
