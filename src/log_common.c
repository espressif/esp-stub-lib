/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <stdarg.h>

extern void ets_printf(const char *fmt, ...);

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
            buf[1] = *fmt;
            buf[2] = '\0';
            ets_printf("%s", buf);
            break;
        }
        fmt++;
    }
    va_end(args);
}
