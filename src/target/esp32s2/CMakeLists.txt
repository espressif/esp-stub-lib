set(srcs
    src/trax.c
    src/uart.c
)

add_library(${ESP_TARGET_LIB} STATIC ${srcs})

target_link_options(${ESP_TARGET_LIB}
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32s2.rom.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32s2.rom.api.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32s2.rom.spiflash_legacy.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32s2.rom.libc-funcs.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32s2.rom.libgcc.ld
)
