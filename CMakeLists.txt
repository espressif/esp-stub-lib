if(NOT DEFINED ESP_TARGET)
    message(FATAL_ERROR "ESP_TARGET not defined. Please specify -DESP_TARGET=<target>")
endif()

if(NOT ESP_STUB_LIB)
    set(ESP_STUB_LIB esp-stub-lib)
endif()

if(NOT STUB_COMMAND)
    set(ESP_BASE_LIB base)
    set(ESP_COMMON_LIB common)
    set(ESP_TARGET_LIB ${ESP_TARGET})
else()
    set(ESP_BASE_LIB base-${STUB_COMMAND})
    set(ESP_COMMON_LIB common-${STUB_COMMAND})
    set(ESP_TARGET_LIB ${ESP_TARGET}-${STUB_COMMAND})
endif()

set(srcs
    src/flash.c
    src/mem_utils.c
    src/rom_wrappers.c
    src/security.c
    src/sha.c
    src/uart.c
    src/usb_serial_jtag.c
    src/usb_otg.c
    src/clock.c
    src/md5.c
)

if(STUB_LOG_ENABLED IN_LIST STUB_COMPILE_DEFS)
    list(APPEND srcs src/log_common.c)

    if(STUB_LIB_LOG_UART IN_LIST STUB_COMPILE_DEFS)
        list(APPEND srcs src/log_uart.c)
    else()
        # STUB_LIB_LOG_BUF is by default
        if(NOT STUB_LIB_LOG_BUF IN_LIST STUB_COMPILE_DEFS)
            list(APPEND STUB_COMPILE_DEFS STUB_LIB_LOG_BUF)
        endif()
        list(APPEND srcs src/log_buf.c)
    endif()
endif()

add_library(${ESP_STUB_LIB} STATIC ${srcs})

target_include_directories(${ESP_STUB_LIB}
    INTERFACE include
    PRIVATE include/esp-stub-lib
)
# Public within the library
include_directories(include)
include_directories(include/esp-stub-lib)

# STUB_COMPILE_DEFS is optional definitions coming from the parent CMakeLists.txt
add_compile_definitions(${STUB_COMPILE_DEFS})

# Three-layer architecture:
#
# Layer structure and locations:
# - entry:  Main stub code                     → src/
# - common: Generic implementations (weak)     → src/target/common/
# - target: Target-specific overrides (strong) → src/target/${ESP_TARGET}/
# - base:   Interface headers only             → src/target/base/
#
# Dependency flow: entry → common → target
#                            ↓         ↓
#                           base      base

add_subdirectory(src/target)

# Link to common library (which transitively includes target and base)
target_link_libraries(${ESP_STUB_LIB} PUBLIC ${ESP_COMMON_LIB})
