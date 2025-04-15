/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <log.h>

#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>

#include <target/esp_rom_caps.h>
#include <target/uart.h>

// These functions are defined in the ROM
extern void ets_install_uart_printf(void);
extern void ets_printf(const char *fmt, ...);
extern void ets_install_putc1(void (*p)(char c));
extern void ets_install_putc2(void (*p)(char c));

static enum stub_lib_log_destination s_log_dest = STUB_LIB_LOG_DEST_BUF;

static struct stub_lib_log_buf g_stub_lib_log_buf;

static void log_buf_write(char c)
{
    g_stub_lib_log_buf.buf[g_stub_lib_log_buf.count] = c;
    g_stub_lib_log_buf.count = (g_stub_lib_log_buf.count + 1) & (STUB_LIB_LOG_BUF_SIZE - 1);
}

void stub_lib_log_init(enum stub_lib_log_destination dest)
{
    if (dest == STUB_LIB_LOG_DEST_UART) {
        stub_target_uart_init(0, 115200);
        //fixme: call ets_install_putc1(0)/putc2(0) here?
        ets_install_uart_printf();
        s_log_dest = STUB_LIB_LOG_DEST_UART;
    } else if (dest == STUB_LIB_LOG_DEST_BUF) {
        ets_install_putc1(log_buf_write);
        ets_install_putc2(NULL);
        s_log_dest = STUB_LIB_LOG_DEST_BUF;
    } else {
        s_log_dest = STUB_LIB_LOG_DEST_NONE;
    }
}

// This function is designed to avoid implementing vprintf() to reduce code size.
// It only supports a subset of format specifiers: %s, %d, %u, %x, %X, %c.
// It does not support floating point numbers or other advanced features.
void stub_lib_log_printf(const char *fmt, ...)
{
    char buf[3];
    va_list args;

    va_start(args, fmt);

    while (*fmt) {
        if (*fmt != '%') {
            ets_printf("%c", *fmt++);
            continue;
        }

        fmt++;  // Skip the '%'

        switch (*fmt) {
        case 's': {
            const char *s = va_arg(args, const char *);
            ets_printf("%s", s ? s : "(null)");
        }
        break;
        case 'd': {
            int d = va_arg(args, int);
            if (d < 0) {
                ets_printf("-%u", (unsigned int)(-d));
            } else {
                ets_printf("%u", (unsigned int)d);
            }
        }
        break;
        case 'u':
        case 'x':
        case 'X':
            buf[0] = '%';
            buf[1] = *fmt;
            buf[2] = '\0';
            ets_printf(buf, va_arg(args, unsigned int));
            break;
        case 'c':
            ets_printf("%c", va_arg(args, int));
            break;
        default:
            buf[0] = '%';
            buf[1] = *fmt ? *fmt++ : '\0';
            buf[2] = '\0';
            ets_printf("%s", buf);
            break;
        }
        fmt++;
    }
    va_end(args);
}
