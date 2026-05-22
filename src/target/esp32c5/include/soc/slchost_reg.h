/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * SLCHOST register definitions for ESP32-C5.
 * Base address: DR_REG_SLCHOST_BASE = 0x60018000  (same as ESP32-C6)
 */

#pragma once

#include <esp-stub-lib/bit_utils.h>

#include <soc/reg_base.h>

/*
 * SLCHOST_CONF_W0_REG (+0x6C):
 * Written by the slave before sending a packet to advertise the total
 * transfer length. The host reads this via CMD53 to know how many bytes
 * to pull before the next packet starts.
 */
#define SLCHOST_CONF_W0_REG (DR_REG_SLCHOST_BASE + 0x6C)

/* Host-visible interrupt registers. */
#define SLCHOST_SLC0HOST_INT_RAW_REG       (DR_REG_SLCHOST_BASE + 0x50)

#define SLCHOST_SLC0_RX_NEW_PACKET_INT BIT(23)
