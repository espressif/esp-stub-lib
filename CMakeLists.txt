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

# Optional defs from the parent (e.g. flasher-stub) plus STUB_LOG options; forwarded to parent at end.
if(DEFINED STUB_COMPILE_DEFS)
    set(_stub_cdefs ${STUB_COMPILE_DEFS})
else()
    set(_stub_cdefs "")
endif()

# STUB_LOG: off by default. Enable: -DSTUB_ENABLE_LOG=ON, then e.g. -DSTUB_LIB_LOG_OUTPUT=UART1|USB_SERIAL_JTAG
set(STUB_ENABLE_LOG OFF CACHE BOOL "Enable STUB_LOG (set STUB_LIB_LOG_OUTPUT when ON)")
set(STUB_LIB_LOG_OUTPUT "UART0" CACHE STRING "When STUB_ENABLE_LOG: UART0, UART1, or USB_SERIAL_JTAG")
set_property(CACHE STUB_LIB_LOG_OUTPUT PROPERTY STRINGS UART0 UART1 USB_SERIAL_JTAG)
if(STUB_ENABLE_LOG)
    list(APPEND _stub_cdefs "STUB_LOG_ENABLED" "STUB_LIB_LOG_UART")
    if(STUB_LIB_LOG_OUTPUT STREQUAL "UART0")
        list(APPEND _stub_cdefs "STUB_LIB_LOG_UART_NUM=0")
    elseif(STUB_LIB_LOG_OUTPUT STREQUAL "UART1")
        list(APPEND _stub_cdefs "STUB_LIB_LOG_UART_NUM=1")
    elseif(STUB_LIB_LOG_OUTPUT STREQUAL "USB_SERIAL_JTAG")
        list(APPEND _stub_cdefs "STUB_LIB_LOG_TO_USB_SERIAL_JTAG=1")
    else()
        message(FATAL_ERROR "STUB_LIB_LOG_OUTPUT must be UART0, UART1, or USB_SERIAL_JTAG (got: ${STUB_LIB_LOG_OUTPUT})")
    endif()
endif()
set(STUB_COMPILE_DEFS ${_stub_cdefs})

set(srcs
    src/flash.c
    src/mem_utils.c
    src/rom_wrappers.c
    src/security.c
    src/sha256.c
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

# Propagate final defs to parent (e.g. flasher-stub `src/CMakeLists.txt` uses them on the stub executable)
set(STUB_COMPILE_DEFS ${STUB_COMPILE_DEFS} PARENT_SCOPE)
