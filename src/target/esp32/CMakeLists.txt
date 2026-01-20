set(srcs
    src/flash.c
    src/trax.c
    src/uart.c
    src/security.c
    src/clock.c
)

add_library(${ESP_TARGET_LIB} STATIC ${srcs})

target_link_options(${ESP_TARGET_LIB}
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32.rom.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32.rom.redefined.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32.rom.libc-funcs.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32.rom.libgcc.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32.rom.api.ld
)
