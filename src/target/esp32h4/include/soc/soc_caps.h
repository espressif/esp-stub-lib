/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

/*-------------------------- COMMON CAPS ---------------------------------------*/
#define SOC_UART_HP_NUM             (2)     /*!< HP UART number */
#define SOC_UART_HAS_SYNC_REG_UPDATE 1

/*-------------------------- SPI FLASH CAPS ------------------------------------*/
#define SOC_SPI_FLASH_4B_ADDR_SUPPORTED 1   /*!< 4-byte (32-bit) flash addressing, needed above 16 MB */
