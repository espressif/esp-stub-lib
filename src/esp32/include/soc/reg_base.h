/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#define DR_REG_DPORT_BASE                       0x3ff00000
#define DR_REG_AES_BASE                         0x3ff01000
#define DR_REG_RSA_BASE                         0x3ff02000
#define DR_REG_SHA_BASE                         0x3ff03000
#define DR_REG_FLASH_MMU_TABLE_PRO              0x3ff10000
#define DR_REG_FLASH_MMU_TABLE_APP              0x3ff12000
#define DR_REG_DPORT_END                        0x3ff13FFC
#define DR_REG_UART_BASE                        0x3ff40000
#define DR_REG_SPI1_BASE                        0x3ff42000
#define DR_REG_SPI0_BASE                        0x3ff43000
#define DR_REG_GPIO_BASE                        0x3ff44000
#define DR_REG_GPIO_SD_BASE                     0x3ff44f00
#define DR_REG_FE2_BASE                         0x3ff45000
#define DR_REG_FE_BASE                          0x3ff46000
#define DR_REG_FRC_TIMER_BASE                   0x3ff47000
#define DR_REG_RTCCNTL_BASE                     0x3ff48000
#define DR_REG_RTCIO_BASE                       0x3ff48400
#define DR_REG_SENS_BASE                        0x3ff48800
#define DR_REG_RTC_I2C_BASE                     0x3ff48C00
#define DR_REG_IO_MUX_BASE                      0x3ff49000
#define DR_REG_HINF_BASE                        0x3ff4B000
#define DR_REG_UHCI1_BASE                       0x3ff4C000
#define DR_REG_I2S_BASE                         0x3ff4F000
#define DR_REG_UART1_BASE                       0x3ff50000
#define DR_REG_BT_BASE                          0x3ff51000
#define DR_REG_I2C_EXT_BASE                     0x3ff53000
#define DR_REG_UHCI0_BASE                       0x3ff54000
#define DR_REG_SLCHOST_BASE                     0x3ff55000
#define DR_REG_RMT_BASE                         0x3ff56000
#define DR_REG_PCNT_BASE                        0x3ff57000
#define DR_REG_SLC_BASE                         0x3ff58000
#define DR_REG_LEDC_BASE                        0x3ff59000
#define DR_REG_EFUSE_BASE                       0x3ff5A000
#define DR_REG_SPI_ENCRYPT_BASE                 0x3ff5B000
#define DR_REG_NRX_BASE                         0x3ff5CC00
#define DR_REG_BB_BASE                          0x3ff5D000
#define DR_REG_PWM0_BASE                        0x3ff5E000
#define DR_REG_TIMERGROUP0_BASE                 0x3ff5F000
#define DR_REG_TIMERGROUP1_BASE                 0x3ff60000
#define DR_REG_RTCMEM0_BASE                     0x3ff61000
#define DR_REG_RTCMEM1_BASE                     0x3ff62000
#define DR_REG_RTCMEM2_BASE                     0x3ff63000
#define DR_REG_SPI2_BASE                        0x3ff64000
#define DR_REG_SPI3_BASE                        0x3ff65000
#define DR_REG_SYSCON_BASE                      0x3ff66000
#define DR_REG_APB_CTRL_BASE                    0x3ff66000    /* Old name for SYSCON, to be removed */
#define DR_REG_I2C1_EXT_BASE                    0x3ff67000
#define DR_REG_SDMMC_BASE                       0x3ff68000
#define DR_REG_EMAC_BASE                        0x3ff69000
#define DR_REG_CAN_BASE                         0x3ff6B000
#define DR_REG_PWM1_BASE                        0x3ff6C000
#define DR_REG_I2S1_BASE                        0x3ff6D000
#define DR_REG_UART2_BASE                       0x3ff6E000
#define PERIPHS_SPI_ENCRYPT_BASEADDR            DR_REG_SPI_ENCRYPT_BASE
