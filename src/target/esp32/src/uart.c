/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <stddef.h>
#include <esp-stub-lib/rom_wrappers.h>
#include <target/uart.h>
#include <target/soc_utils.h>
#include <soc/uart_reg.h>

// These functions are defined in the ROM
extern void uartAttach(void);
extern void Uart_Init(uint8_t uart_no, uint32_t clock);
extern void uart_rx_intr_handler(void *para);
extern void ets_isr_attach(int int_num, void *handler, void *arg);
extern void ets_isr_unmask(int int_num);
extern void uart_tx_switch(uint8_t uart_no);
extern void uart_div_modify(uint8_t uart_no, uint32_t divisor);
extern uint32_t ets_get_detected_xtal_freq(void);
extern void uart_tx_wait_idle(uint8_t uart_no);

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
    uint32_t clock = ets_get_detected_xtal_freq();

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

void stub_target_uart_init(uint8_t uart_num, uint32_t baudrate)
{
    uartAttach();
    Uart_Init(uart_num, ets_get_detected_xtal_freq_patch());
    uart_tx_switch(uart_num);
    stub_target_uart_set_baudrate(uart_num, baudrate);

    // Clear all pending interrupts
    WRITE_PERI_REG(UART_INT_CLR_REG(uart_num), 0xFFFFFFFFU);
}

void stub_target_uart_set_baudrate(uint8_t uart_num, uint32_t baudrate)
{
    uint32_t clock = ets_get_detected_xtal_freq_patch() << 4;
    uint32_t divisor = clock / baudrate;
    uart_tx_wait_idle(uart_num);
    uart_div_modify(uart_num, divisor);
}

void stub_target_uart_intr_attach(uint8_t uart_num, int intr_num, void *handler, uint32_t flags)
{
    // Enable requested interrupts
    if (flags != 0U) {
        SET_PERI_REG_MASK(UART_INT_ENA_REG(uart_num), flags);
    }

    // Attach and unmask the interrupt handler
    ets_isr_attach(intr_num, handler, NULL);
    ets_isr_unmask(1 << intr_num);
}

uint32_t stub_target_uart_get_intr_flags(uint8_t uart_num)
{
    uint32_t status = READ_PERI_REG(UART_INT_ST_REG(uart_num));
    // Clear the interrupts
    WRITE_PERI_REG(UART_INT_CLR_REG(uart_num), status);
    return status;
}

uint32_t stub_target_uart_get_rxfifo_count(uint8_t uart_num)
{
    uint32_t status = READ_PERI_REG(UART_STATUS_REG(uart_num));
    return (status >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT_V;
}

uint8_t stub_target_uart_read_rxfifo_byte(uint8_t uart_num)
{
    return (uint8_t)(READ_PERI_REG(UART_FIFO_REG(uart_num)) & UART_RXFIFO_RD_BYTE_V);
}

void stub_target_uart_set_rx_timeout(uint8_t uart_num, uint8_t timeout)
{
    uint32_t conf1 = READ_PERI_REG(UART_CONF1_REG(uart_num));
    conf1 &= ~(UART_RX_TOUT_THRHD_M | UART_RX_TOUT_EN_M);

    if (timeout > 0U) {
        conf1 |= ((timeout & UART_RX_TOUT_THRHD_V) << UART_RX_TOUT_THRHD_S) | UART_RX_TOUT_EN;
    }

    WRITE_PERI_REG(UART_CONF1_REG(uart_num), conf1);
}
