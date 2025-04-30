set(COMMON_SRCS src/log_printf.c)

if(STUB_LOG_ENABLED IN_LIST STUB_COMPILE_DEFS)
    if(STUB_LIB_LOG_UART IN_LIST STUB_COMPILE_DEFS)
        list(APPEND COMMON_SRCS src/log_uart.c)
    else()
        # STUB_LIB_LOG_BUF is by default
        if(NOT STUB_LIB_LOG_BUF IN_LIST STUB_COMPILE_DEFS)
            list(APPEND STUB_COMPILE_DEFS STUB_LIB_LOG_BUF)
        endif()
        list(APPEND COMMON_SRCS src/log_buf.c)
    endif()
endif()

add_library(${ESP_COMMON_LIB} STATIC ${COMMON_SRCS})

target_include_directories(${ESP_COMMON_LIB}
    PUBLIC include
    PRIVATE include/common
)

target_compile_definitions(${ESP_COMMON_LIB} PRIVATE ${STUB_COMPILE_DEFS})
