/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#if defined(STUB_LOG_ENABLED)

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void stub_lib_log_init();
void stub_lib_log_printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif // __cplusplus

#define STUB_LOG_INIT() stub_lib_log_init()
#define STUB_LOG(fmt, ...) stub_lib_log_printf(fmt, ##__VA_ARGS__)

#else // defined(STUB_LOG_ENABLED)

#define STUB_LOG_INIT() do {} while(0)
#define STUB_LOG(fmt, ...) do {} while(0)

#endif // defined(STUB_LOG_ENABLED)

#define STUB_LOGE(fmt, ...) STUB_LOG("STUB_E: " fmt, ##__VA_ARGS__)
#define STUB_LOGW(fmt, ...) STUB_LOG("STUB_W: " fmt, ##__VA_ARGS__)
#define STUB_LOGI(fmt, ...) STUB_LOG("STUB_I: " fmt, ##__VA_ARGS__)
#define STUB_LOGD(fmt, ...) STUB_LOG("STUB_D: " fmt, ##__VA_ARGS__)
#define STUB_LOGV(fmt, ...) STUB_LOG("STUB_V: " fmt, ##__VA_ARGS__)
// trace only function name
#define STUB_LOG_TRACE() STUB_LOG("STUB_T: %s()\n", __func__)
// trace with format
#define STUB_LOG_TRACEF(fmt, ...) STUB_LOG("STUB_T: %s(): " fmt, __func__, ##__VA_ARGS__)
