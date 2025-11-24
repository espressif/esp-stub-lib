set(srcs
    src/uart.c
)

add_library(${ESP_TARGET_LIB} STATIC ${srcs})

target_link_options(${ESP_TARGET_LIB}
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32c5.rom.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32c5.rom.api.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32c5.rom.extra.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32c5.rom.libc.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32c5.rom.libgcc.ld
)
