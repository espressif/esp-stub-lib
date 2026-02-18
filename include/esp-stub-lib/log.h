/*
 * SPDX-FileCopyrightText: 2025-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

typedef enum {
    STUB_LOG_LEVEL_NONE = 0,
    STUB_LOG_LEVEL_E = 1,
    STUB_LOG_LEVEL_W = 2,
    STUB_LOG_LEVEL_I = 3,
    STUB_LOG_LEVEL_D = 4,
    STUB_LOG_LEVEL_T = 5,
    STUB_LOG_LEVEL_TRACE = STUB_LOG_LEVEL_T,
    STUB_LOG_LEVEL_V = 6,
} stub_lib_log_level_t;

#if !defined(STUB_LIB_LOG_LEVEL)
#define STUB_LIB_LOG_LEVEL STUB_LOG_LEVEL_I
#endif

#if defined(STUB_LOG_ENABLED)

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief Initialize logging backend and set the initial log level.
 *
 * @param level Initial log level.
 */
void stub_lib_log_init(stub_lib_log_level_t level);

/**
 * @brief Set the current runtime log level.
 *
 * @param level New log level.
 */
void stub_lib_log_set_level(stub_lib_log_level_t level);

/**
 * @brief Check whether a log level is currently enabled.
 *
 * @param level Log level to check.
 * @return Non-zero if enabled, zero otherwise.
 */
int stub_lib_log_level_enabled(stub_lib_log_level_t level);

/**
 * @brief Print a formatted log message.
 *
 * @param fmt Format string.
 *
 * @note Supported format specifiers are: `%s`, `%d`, `%u`, `%x`, `%X`, `%c`.
 *       Other specifiers are not supported.
 */
void stub_lib_log_printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif // __cplusplus

#define STUB_LOG_INIT(level) stub_lib_log_init(level)
#define STUB_LOG(fmt, ...)   stub_lib_log_printf(fmt, ##__VA_ARGS__)

#else // defined(STUB_LOG_ENABLED)

#define STUB_LOG_INIT(level)                                                                                           \
    do {                                                                                                               \
        (void)(level);                                                                                                 \
    } while (0)
#define STUB_LOG(fmt, ...)                                                                                             \
    do {                                                                                                               \
    } while (0)

#endif // defined(STUB_LOG_ENABLED)

#if defined(STUB_LOG_ENABLED)
#define STUB_LOG_AT_LEVEL(level, fmt, ...)                                                                             \
    do {                                                                                                               \
        if (stub_lib_log_level_enabled(level)) {                                                                       \
            STUB_LOG(fmt, ##__VA_ARGS__);                                                                              \
        }                                                                                                              \
    } while (0)
#else
#define STUB_LOG_AT_LEVEL(level, fmt, ...)                                                                             \
    do {                                                                                                               \
    } while (0)
#endif

#define STUB_LOGE(fmt, ...)       STUB_LOG_AT_LEVEL(STUB_LOG_LEVEL_E, "STUB_E: " fmt, ##__VA_ARGS__)
#define STUB_LOGW(fmt, ...)       STUB_LOG_AT_LEVEL(STUB_LOG_LEVEL_W, "STUB_W: " fmt, ##__VA_ARGS__)
#define STUB_LOGI(fmt, ...)       STUB_LOG_AT_LEVEL(STUB_LOG_LEVEL_I, "STUB_I: " fmt, ##__VA_ARGS__)
#define STUB_LOGD(fmt, ...)       STUB_LOG_AT_LEVEL(STUB_LOG_LEVEL_D, "STUB_D: " fmt, ##__VA_ARGS__)
#define STUB_LOGV(fmt, ...)       STUB_LOG_AT_LEVEL(STUB_LOG_LEVEL_V, "STUB_V: " fmt, ##__VA_ARGS__)
// trace only function name
#define STUB_LOG_TRACE()          STUB_LOG_AT_LEVEL(STUB_LOG_LEVEL_T, "STUB_T: %s()\n", __func__)
// trace with format
#define STUB_LOG_TRACEF(fmt, ...) STUB_LOG_AT_LEVEL(STUB_LOG_LEVEL_T, "STUB_T: %s(): " fmt, __func__, ##__VA_ARGS__)
