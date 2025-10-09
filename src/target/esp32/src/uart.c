/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <stddef.h>
#include <target/uart.h>
#include <soc_utils.h>

#include <soc/uart_reg.h>

// These functions are defined in the ROM
extern void esp_rom_uart_attach(void);
extern void esp_rom_uart_init(uint8_t uart_no, uint32_t clock);
extern void esp_rom_uart_rx_intr_handler(void *para);
extern void esp_rom_isr_attach(int int_num, void *handler, void *arg);
extern void esp_rom_isr_unmask(int int_num);
extern void esp_rom_uart_set_as_console(uint8_t uart_no);
extern void esp_rom_uart_div_modify(uint8_t uart_no, uint32_t divisor);
extern uint32_t esp_rom_get_detected_xtal_freq(void);

#define RTC_STORE5 (DR_REG_RTCCNTL_BASE + 0xb4)

/*
 * The symbol 'ets_get_detected_xtal_freq_patch' is defined as:
 *   PROVIDE ( ets_get_detected_xtal_freq_patch = 40065c18 );
 *
 * This symbol exists in ROM revision 3 but is absent in earlier revisions.
 * It is not expected to have a significant impact on the stub size.
 */
static uint32_t ets_get_detected_xtal_freq_patch()
{
    // Original detection function, reads from RTC_STORE5
    // or CK8 detected frequency otherwise
    uint32_t clock = esp_rom_get_detected_xtal_freq();

    // Quantize the detected clock to either 40MHz or 26MHz
    // mid-point is 30.5MHz based on data from ATE team about
    // 8M clock error rates
    if (clock < 30500 * 1000) {
        clock = 26 * 1000 * 1000;
    } else {
        clock = 40 * 1000 * 1000;
    }

    const uint32_t rtc_store5 = REG_READ(RTC_STORE5);
    const uint32_t high = rtc_store5 >> 16;
    const uint32_t low = rtc_store5 & 0xffff;
    if (high == low && high != 0 && high != 0xffff) {
        clock = high << 12;
    } else {
        REG_WRITE(RTC_STORE5, (clock >> 12) | ((clock >> 12) << 16));
    }

    return clock;
}

void stub_target_rom_uart_attach(void *rxBuffer)
{
    (void)rxBuffer;  // ESP32 ROM doesn't take parameter
    esp_rom_uart_attach();
}

void stub_target_rom_uart_init(uint8_t uart_no, uint32_t clock)
{
    esp_rom_uart_init(uart_no, clock);
}

void stub_target_uart_init(uint8_t uart_num)
{
    stub_target_rom_uart_attach(NULL);
    stub_target_rom_uart_init(uart_num, ets_get_detected_xtal_freq_patch());
    esp_rom_uart_set_as_console(uart_num);
}

void stub_target_uart_rominit_set_baudrate(uint8_t uart_num, uint32_t baudrate)
{
    uint32_t clock = ets_get_detected_xtal_freq_patch() << 4;
    uint32_t divisor = clock / baudrate;
    stub_target_uart_wait_idle(uart_num);
    esp_rom_uart_div_modify(uart_num, divisor);
}
