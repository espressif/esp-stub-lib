/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <stdbool.h>

/**
 * @brief Check if USB-OTG is supported on this chip
 *
 * @return true if USB-OTG is supported, false otherwise
 */
bool stub_target_usb_otg_is_supported(void);

/**
 * @brief Initialize USB-OTG for communication
 *
 * This function sets up USB-OTG interrupts and the callback function.
 *
 * @param intr_num Interrupt number
 * @param callback Callback function to be called when data is received
 */
void stub_target_usb_otg_rominit_intr_attach(int intr_num, void *callback);

/**
 * @brief Handle USB-OTG reset request
 *
 * This function performs the necessary cleanup and performs a software reset.
 */
void stub_target_usb_otg_handle_reset(void);
