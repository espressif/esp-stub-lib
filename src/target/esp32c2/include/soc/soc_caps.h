/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

/*-------------------------- COMMON CAPS ---------------------------------------*/
#define SOC_UART_HP_NUM             (2)     /*!< HP UART number */

// CPU Caps
#define SOC_CPU_HAS_CSR_PC          1       /*!< ROM delays read the cycle count from the mpccr CSR, not mcycle */
