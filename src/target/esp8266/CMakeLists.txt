set(srcs
    src/uart.c
    src/rom-patched.c
    src/security.c
    src/flash.c
)

add_library(${ESP_TARGET_LIB} STATIC ${srcs})

target_link_options(${ESP_TARGET_LIB}
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp8266.rom.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp8266.rom.api.ld)
