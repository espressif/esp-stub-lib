/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include <esp-stub-lib/log.h>
#include <esp-stub-lib/flash.h>

#include "stub_lib_err.h"

#include "stub_main.h"

extern uint32_t _bss_start;
extern uint32_t _bss_end;

struct stub_cmd_handler {
    int cmd;
    const char *name;
    int (*handler)(va_list ap);
};

static  __attribute__((unused)) int handle_test1(va_list ap)
{
    (void)ap;

    STUB_LOG("stub command test:%d\n", 1);
    STUB_LOG("stub command test:%d\n", -1);
    STUB_LOG("stub command test:0x%x\n", 0x4080393C);
    STUB_LOG("stub command test:%s\n", "test");
    STUB_LOG("stub command test:%c\n", 'A');
    STUB_LOG("stub command test:%l\n", 10); // not supported

    STUB_LOGE("stub command test\n");
    STUB_LOGW("stub command test\n");
    STUB_LOGI("stub command test\n");
    STUB_LOGD("stub command test\n");
    STUB_LOGV("stub command test\n");
    STUB_LOG_TRACE();
    STUB_LOG_TRACEF("foo:%u\n", 0x2A);

    // test the compiler runtime that placed in ROM
    extern int32_t __bswapsi2(int32_t x);
    (void)__bswapsi2(0x77AAFF33);

    return 0;
}

static  __attribute__((unused)) int handle_test2(va_list ap)
{
    (void)ap;

    char buf[10];
    strcpy(buf, "test2\n");
    STUB_LOG(buf);

    return 0;
}

static const struct stub_cmd_handler cmd_handlers[] = {
    {ESP_STUB_CMD_TEST1, "CMD_TEST1", handle_test1},
    {ESP_STUB_CMD_TEST2, "CMD_TEST2", handle_test2},
    {0, NULL, NULL}
};

int stub_main(int cmd, ...) __attribute__((used));

#ifdef ESP8266
__asm__(
    ".global stub_main_esp8266\n"
    ".literal_position\n"
    ".align 4\n"
    "stub_main_esp8266:\n"
    "movi a0, 0x400010a8;"
    "j stub_main;");
#endif

int stub_main(int cmd, ...)
{
    va_list ap;
    void *flash_state = NULL;
    int ret = ESP_STUB_FAIL;
    stub_lib_err_t rc = STUB_LIB_FAIL;

    /* zero bss */
    for (uint32_t *p = &_bss_start; p < &_bss_end; p++) {
        *p = 0;
    }

    va_start(ap, cmd);

    STUB_LOG_INIT();

    rc = stub_lib_flash_init(&flash_state);
    if (rc != STUB_LIB_OK) {
        return ESP_STUB_FAIL;
    }

    STUB_LOGI("Command: 0x%x\n", cmd);

    const struct stub_cmd_handler *handler = cmd_handlers;
    while (handler->handler) {
        if (handler->cmd == cmd) {
            STUB_LOGI("Executing command: %s\n", handler->name);
            ret = handler->handler(ap);
            if (ret != ESP_STUB_OK) {
                STUB_LOGE("Command %s (0x%x) failed\n", handler->name, handler->cmd);
                goto flash_va_end;
            }
            break;
        }
        handler++;
    }

    if (!handler->handler) {
        STUB_LOGE("Unknown command (0x%x)!\n", cmd);
    }

flash_va_end:
    va_end(ap);

    if (flash_state) {
        stub_lib_flash_deinit(flash_state);
    }

    return ret;
}
