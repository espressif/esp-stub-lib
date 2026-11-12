/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

/*
 * 4-byte address flash commands configured with the ROM's
 * esp_rom_spi_set_op_mode() and esp_rom_spi_cmd_config(), which have the same
 * address on every chip revision, and started through the SPI controller
 * registers. Reads and writes configure the controller once and then start one
 * command per 64 bytes. Built only by targets whose ROM provides these
 * functions; others use the defaults in flash_4byte.c.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <esp-stub-lib/err.h>
#include <esp-stub-lib/flash.h>
#include <esp-stub-lib/soc_utils.h>

#include <target/flash.h>
#include <target/flash_4byte.h>

#include <private/flash_commands.h>
#include <private/rom_flash.h>
#include <private/rom_flash_config.h>

#include <soc/spi_mem_compat.h>

// Size of the SPI data buffer
#define SPI_DATA_BUF_SIZE 64

extern void *memcpy(void *dest, const void *src, size_t n);

extern esp_rom_spiflash_legacy_data_t *rom_spiflash_legacy_data;

// Registers that esp_rom_opiflash_exec_cmd() restores after each command
typedef struct {
    uint32_t user;
    uint32_t user1;
    uint32_t user2;
    uint32_t ctrl;
    uint32_t ddr;
} user_cmd_regs_t;

static __attribute__((noinline)) void user_cmd_regs_save(int spi_num, user_cmd_regs_t *regs)
{
    (void)spi_num;
    regs->user = READ_PERI_REG(SPI_MEM_USER_REG(spi_num));
    regs->user1 = READ_PERI_REG(SPI_MEM_USER1_REG(spi_num));
    regs->user2 = READ_PERI_REG(SPI_MEM_USER2_REG(spi_num));
    regs->ctrl = READ_PERI_REG(SPI_MEM_CTRL_REG(spi_num));
    regs->ddr = READ_PERI_REG(SPI_MEM_DDR_REG(spi_num));
}

static __attribute__((noinline)) void user_cmd_regs_load(int spi_num, const user_cmd_regs_t *regs)
{
    (void)spi_num;
    WRITE_PERI_REG(SPI_MEM_USER_REG(spi_num), regs->user);
    WRITE_PERI_REG(SPI_MEM_USER1_REG(spi_num), regs->user1);
    WRITE_PERI_REG(SPI_MEM_USER2_REG(spi_num), regs->user2);
    WRITE_PERI_REG(SPI_MEM_CTRL_REG(spi_num), regs->ctrl);
    WRITE_PERI_REG(SPI_MEM_DDR_REG(spi_num), regs->ddr);
}

// Configure a command as esp_rom_opiflash_exec_cmd() does, without starting it
static __attribute__((noinline)) void user_cmd_config(const opiflash_cmd_params_t *params)
{
    uint32_t addr = params->addr;
    esp_rom_spi_cmd_t conf = {
        .cmd = (uint16_t)params->cmd,
        .cmd_bit_len = (uint16_t)params->cmd_bit_len,
        .addr = &addr,
        .addr_bit_len = (uint32_t)params->addr_bit_len,
        .tx_data = (uint32_t *)(uintptr_t)params->mosi_data,
        .tx_data_bit_len = (uint32_t)params->mosi_bit_len,
        .rx_data_bit_len = (uint32_t)params->miso_bit_len,
        .dummy_bit_len = (uint32_t)params->dummy_bits + rom_spiflash_legacy_data->dummy_len_plus[params->spi_num],
    };
    esp_rom_spi_set_op_mode(params->spi_num, params->mode);
    esp_rom_spi_cmd_config(params->spi_num, &conf);
}

static __attribute__((noinline)) void user_cmd_run(int spi_num, uint32_t flash_addr, uint32_t cmd_bits)
{
    (void)spi_num;
    WRITE_PERI_REG(SPI_MEM_ADDR_REG(spi_num), flash_addr);
    WRITE_PERI_REG(SPI_MEM_CMD_REG(spi_num), cmd_bits);
    while (READ_PERI_REG(SPI_MEM_CMD_REG(spi_num)) != 0) {
    }
}

void stub_target_flash_4byte_read_data(int spi_num, uint32_t flash_addr, uint8_t *buffer, uint32_t size, bool dout)
{
    user_cmd_regs_t saved;
    user_cmd_regs_save(spi_num, &saved);

    // Both reads take 8 dummy cycles; dual output sends the command and address on one line
    user_cmd_config(&(opiflash_cmd_params_t){ .spi_num = spi_num,
                                              .mode = dout ? SPI_FLASH_DOUT_MODE : SPI_FLASH_FASTRD_MODE,
                                              .cmd = dout ? CMD_DOUT_READ_4B : CMD_FSTRD4B,
                                              .cmd_bit_len = 8,
                                              .addr_bit_len = 32,
                                              .dummy_bits = 8,
                                              .miso_bit_len = 8 * SPI_DATA_BUF_SIZE });

    uint32_t len = SPI_DATA_BUF_SIZE;
    while (size > 0) {
        uint32_t chunk_size = (size > SPI_DATA_BUF_SIZE) ? SPI_DATA_BUF_SIZE : size;
        if (chunk_size != len) {
            len = chunk_size;
            REG_SET_FIELD(SPI_MEM_MISO_DLEN_REG(spi_num), SPI_MEM_USR_MISO_DBITLEN, 8 * len - 1);
        }
        user_cmd_run(spi_num, flash_addr, SPI_MEM_USR);

        for (uint32_t i = 0; i < chunk_size; i += 4) {
            uint32_t word = READ_PERI_REG((uint32_t)SPI_MEM_W0_REG(spi_num) + i);
            memcpy(buffer + i, &word, (chunk_size - i < 4) ? chunk_size - i : 4);
        }

        size -= chunk_size;
        buffer += chunk_size;
        flash_addr += chunk_size;
    }

    user_cmd_regs_load(spi_num, &saved);
}

int stub_target_flash_4byte_program(int spi_num,
                                    uint32_t flash_addr,
                                    const uint8_t *data,
                                    uint32_t size,
                                    uint64_t timeout_us)
{
    if (stub_lib_flash_wait_ready(timeout_us) != STUB_LIB_OK) {
        return STUB_LIB_ERR_TIMEOUT;
    }

    user_cmd_regs_t saved;
    user_cmd_regs_save(spi_num, &saved);
    user_cmd_config(&(opiflash_cmd_params_t){ .spi_num = spi_num,
                                              .mode = SPI_FLASH_FASTRD_MODE,
                                              .cmd = CMD_PROGRAM_PAGE_4B,
                                              .cmd_bit_len = 8,
                                              .addr_bit_len = 32,
                                              .mosi_bit_len = 8 * SPI_DATA_BUF_SIZE });

    int ret = STUB_LIB_OK;
    while (size > 0) {
        uint32_t page_space = STUB_FLASH_PAGE_SIZE - (flash_addr % STUB_FLASH_PAGE_SIZE);
        uint32_t chunk_size = (size > SPI_DATA_BUF_SIZE) ? SPI_DATA_BUF_SIZE : size;
        chunk_size = (chunk_size > page_space) ? page_space : chunk_size;

        user_cmd_run(spi_num, flash_addr, SPI_MEM_FLASH_WREN);
        REG_SET_FIELD(SPI_MEM_MOSI_DLEN_REG(spi_num), SPI_MEM_USR_MOSI_DBITLEN, 8 * chunk_size - 1);
        for (uint32_t i = 0; i < chunk_size; i += 4) {
            uint32_t word = 0;
            memcpy(&word, data + i, (chunk_size - i < 4) ? chunk_size - i : 4);
            WRITE_PERI_REG((uint32_t)SPI_MEM_W0_REG(spi_num) + i, word);
        }
        user_cmd_run(spi_num, flash_addr, SPI_MEM_USR | SPI_MEM_FLASH_PE);

        if (stub_lib_flash_wait_ready(timeout_us) != STUB_LIB_OK) {
            ret = STUB_LIB_ERR_TIMEOUT;
            break;
        }

        data += chunk_size;
        flash_addr += chunk_size;
        size -= chunk_size;
    }

    user_cmd_regs_load(spi_num, &saved);
    return ret;
}

// Only chip select 0 is supported; the stub never changes the chip select setting
void stub_target_opiflash_exec_cmd(const opiflash_cmd_params_t *params)
{
    user_cmd_regs_t saved;
    user_cmd_regs_save(params->spi_num, &saved);
    user_cmd_config(params);
    user_cmd_run(params->spi_num,
                 params->addr,
                 params->is_write_erase_operation ? (SPI_MEM_USR | SPI_MEM_FLASH_PE) : SPI_MEM_USR);
    if (params->miso_data != NULL) {
        memcpy(params->miso_data, (const void *)SPI_MEM_W0_REG(params->spi_num), (size_t)params->miso_bit_len / 8);
    }
    user_cmd_regs_load(params->spi_num, &saved);
}
