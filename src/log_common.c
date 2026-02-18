/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#include <stdarg.h>

#include <esp-stub-lib/log.h>

extern void ets_printf(const char *fmt, ...);
extern void stub_lib_log_backend_init(void);

static stub_lib_log_level_t g_stub_lib_log_level = STUB_LIB_LOG_LEVEL;

static stub_lib_log_level_t stub_lib_log_level_normalize(stub_lib_log_level_t level)
{
    if (level < STUB_LOG_LEVEL_NONE) {
        return STUB_LOG_LEVEL_NONE;
    }

    if (level > STUB_LOG_LEVEL_V) {
        return STUB_LOG_LEVEL_V;
    }

    return level;
}

void stub_lib_log_set_level(stub_lib_log_level_t level)
{
    g_stub_lib_log_level = stub_lib_log_level_normalize(level);
}

int stub_lib_log_level_enabled(stub_lib_log_level_t level)
{
    return g_stub_lib_log_level >= level;
}

void stub_lib_log_init(stub_lib_log_level_t level)
{
    stub_lib_log_set_level(level);
    stub_lib_log_backend_init();
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

        fmt++; // Skip the '%'

        switch (*fmt) {
        case 's': {
            const char *s = va_arg(args, const char *);
            ets_printf("%s", s ? s : "(null)");
            break;
        }
        case 'd': {
            int d = va_arg(args, int);
            if (d < 0) {
                ets_printf("-%u", (unsigned int)(-d));
            } else {
                ets_printf("%u", (unsigned int)d);
            }
            break;
        }
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
