/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include <esp-stub-lib/clock.h>
#include <esp-stub-lib/log.h>
#include <esp-stub-lib/flash.h>
#include <esp-stub-lib/err.h>
#include <esp-stub-lib/mem_utils.h>
#include <esp-stub-lib/security.h>
#include <esp-stub-lib/uart.h>
#include <esp-stub-lib/usb_serial_jtag.h>
#include <esp-stub-lib/rom_wrappers.h>
#include <esp-stub-lib/miniz.h>
#include <esp-stub-lib/md5.h>

#include "stub_main.h"

extern uint32_t _bss_start;
extern uint32_t _bss_end;

struct stub_cmd_handler {
    int cmd;
    const char *name;
    int (*handler)(va_list ap);
};

static void example_mem_utils(void)
{
    (void)stub_lib_mem_is_irom(0x0);
    (void)stub_lib_mem_is_drom(0x0);
    (void)stub_lib_mem_is_iram(0x0);
    (void)stub_lib_mem_is_dram(0x0);
    (void)stub_lib_mem_is_rtc_iram_fast(0x0);
    (void)stub_lib_mem_is_rtc_dram_fast(0x0);
    (void)stub_lib_mem_is_rtc_slow(0x0);
    (void)stub_lib_mem_is_tcm(0x0);
}

static void example_security(void)
{
    bool encryption_enabled = stub_lib_security_flash_is_encrypted();
    STUB_LOGI("Flash encryption enabled: %d\n", encryption_enabled);

    uint32_t size = stub_lib_security_info_size();
    STUB_LOGI("Security info size: %u bytes\n", size);

    if (size > 0) {
        uint8_t security_buffer[size];

        int ret = stub_lib_get_security_info(security_buffer, size);
        if (ret == STUB_LIB_OK) {
            STUB_LOGI("Security info retrieved successfully\n");
            STUB_LOGI("Security info: ");
            for (uint32_t i = 0; i < size; ++i) {
                STUB_LOG("0x%02x ", security_buffer[i]);
            }
            STUB_LOG("\n\n");
        } else {
            STUB_LOGE("Failed to get security info: %d\n", ret);
        }
    } else {
        STUB_LOGI("Security info not supported on this chip\n");
    }
}

static void __attribute__((unused)) test_clock_init(void)
{
    stub_lib_clock_init();
    stub_lib_clock_disable_watchdogs();
}

static __attribute__((unused)) void test_miniz(void)
{
    tinfl_decompressor inflator;
    tinfl_init(&inflator);
    tinfl_decompress(&inflator, 0, NULL, 0, 0, NULL, 0);
}

static void __attribute__((unused)) test_usb_serial_jtag(void)
{
    (void)stub_lib_usb_serial_jtag_is_active();
    (void)stub_lib_usb_serial_jtag_rominit_intr_attach(17, NULL, 0);
    (void)stub_lib_usb_serial_jtag_clear_intr_flags();
    (void)stub_lib_usb_serial_jtag_is_data_available();
    (void)stub_lib_usb_serial_jtag_read_rxfifo_byte();
    (void)stub_lib_usb_serial_jtag_tx_one_char('A');
    (void)stub_lib_usb_serial_jtag_tx_flush();
}

static __attribute__((unused)) void test_md5(void)
{
    struct stub_lib_md5_ctx ctx;
    stub_lib_md5_init(&ctx);
    stub_lib_md5_update(&ctx, NULL, 0);
    stub_lib_md5_final(&ctx, NULL);
}

static int __attribute__((unused)) handle_test_uart(void)
{
    void *uart_rx_interrupt_handler = NULL;
    stub_lib_uart_wait_idle(UART_NUM_0);

    stub_lib_uart_init(UART_NUM_0);
    stub_lib_uart_set_rx_timeout(UART_NUM_0, 10);
    (void)stub_lib_uart_get_rxfifo_count(UART_NUM_0);
    (void)stub_lib_uart_clear_intr_flags(UART_NUM_0);
    (void)stub_lib_uart_read_rxfifo_byte(UART_NUM_0);
    (void)stub_lib_uart_rx_one_char();
    stub_lib_uart_rominit_intr_attach(UART_NUM_0,
                                      5,
                                      uart_rx_interrupt_handler,
                                      UART_INTR_RXFIFO_FULL | UART_INTR_RXFIFO_TOUT);
    stub_lib_uart_tx_one_char('A');
    stub_lib_uart_tx_flush(UART_NUM_0);

    return 0;
}

static int __attribute__((unused)) handle_test_flash(void)
{
    void *flash_state = NULL;
    stub_lib_flash_config_t flash_config;
    uint8_t buffer[256];

    (void)stub_lib_flash_init(&flash_state);
    stub_lib_flash_get_config(&flash_config);
    (void)stub_lib_flash_update_config(&flash_config);
    stub_lib_flash_attach(0, false);
    (void)stub_lib_flash_read_buff(0x1000, buffer, sizeof(buffer));
    (void)stub_lib_flash_write_buff(0x10000, buffer, sizeof(buffer), 0);
    (void)stub_lib_flash_erase_area(0x20000, 0x10000);
    (void)stub_lib_flash_erase_chip();
    (void)stub_lib_flash_start_next_erase(NULL, NULL);
    (void)stub_lib_flash_wait_ready(0);
    stub_lib_flash_deinit(flash_state);

    return 0;
}

static __attribute__((unused)) int handle_test1(va_list ap)
{
    (void)ap;

    STUB_LOG("stub command test:%d\n", 1);
    STUB_LOG("stub command test:%d\n", -1);
    STUB_LOG("stub command test:0x%x\n", 0x4080393C);
    STUB_LOG("stub command test:%s\n", "test");
    STUB_LOG("stub command test:%c\n", 'A');
    STUB_LOG("stub command test:%l\n", 10); // not supported

    STUB_LOGE("This is an error message\n");
    STUB_LOGW("This is a warning message\n");
    STUB_LOGI("This is an info message\n");

    stub_lib_log_set_level(STUB_LOG_LEVEL_V);
    STUB_LOGD("This is a debug message\n");
    STUB_LOGV("This is a verbose message\n");
    STUB_LOG_TRACE();
    STUB_LOG_TRACEF("foo:%u\n", 0x2A);

    // test the compiler runtime that placed in ROM
    extern int32_t __bswapsi2(int32_t x);
    (void)__bswapsi2(0x77AAFF33);

    example_mem_utils();
    example_security();

    return 0;
}

static __attribute__((unused)) int handle_test2(va_list ap)
{
    (void)ap;

    char buf[10];
    strcpy(buf, "test2\n");
    STUB_LOG(buf);

    return 0;
}

static const struct stub_cmd_handler cmd_handlers[] = { { ESP_STUB_CMD_TEST1, "CMD_TEST1", handle_test1 },
                                                        { ESP_STUB_CMD_TEST2, "CMD_TEST2", handle_test2 },
                                                        { 0, NULL, NULL } };

int stub_main(int cmd, ...) __attribute__((used));

#ifdef ESP8266
__asm__(".global stub_main_esp8266\n"
        ".literal_position\n"
        ".align 4\n"
        "stub_main_esp8266:\n"
        "movi a0, 0x400010a8;"
        "j stub_main;");
#endif

const char *stub_err_str(int ret_code)
{
    switch (ret_code) {
    case ESP_STUB_FAIL:
        return "Generic error";
    case ESP_STUB_OK:
        return "Success";

    case STUB_LIB_FAIL:
        return "Failure in the lib";
    case STUB_LIB_ERR_UNKNOWN_FLASH_ID:
        return "Unknown flash ID";

    default:
        return "Unknown stub error";
    }
}

int stub_main(int cmd, ...)
{
    va_list ap;
    void *flash_state = NULL;
    int ret = ESP_STUB_FAIL;

    /* zero bss */
    for (uint32_t *p = &_bss_start; p < &_bss_end; p++) {
        *p = 0;
    }

    va_start(ap, cmd);

    STUB_LOG_INIT(STUB_LIB_LOG_LEVEL);

    STUB_LOGI("Command: 0x%x\n", cmd);

    int lib_ret = stub_lib_flash_init(&flash_state);
    if (lib_ret != STUB_LIB_OK) {
        STUB_LOGE("Flash init failure: (0x%X) %s\n", lib_ret, stub_err_str(lib_ret));
        return lib_ret;
    }

    const struct stub_cmd_handler *handler = cmd_handlers;
    while (handler->handler) {
        if (handler->cmd == cmd) {
            STUB_LOGI("Executing command: %s\n", handler->name);
            ret = handler->handler(ap);
            if (ret != ESP_STUB_OK) {
                STUB_LOGE("Command %s (0x%x) failed: (0x%X) %s\n", handler->name, handler->cmd, ret, stub_err_str(ret));
                goto flash_va_end;
            }
            break;
        }
        handler++;
    }

    if (!handler->handler) {
        STUB_LOGE("Unknown command (0x%x)!\n", cmd);
        ret = ESP_STUB_ERR_CMD_NOT_SUPPORTED;
    }

flash_va_end:
    va_end(ap);

    if (flash_state) {
        stub_lib_flash_deinit(flash_state);
    }

    return ret;
}
