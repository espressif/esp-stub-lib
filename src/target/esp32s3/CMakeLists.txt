set(srcs
    src/flash.c
    src/trax.c
    src/uart.c
)

add_library(${ESP_TARGET_LIB} STATIC ${srcs})

target_include_directories(${ESP_TARGET_LIB}
    PUBLIC include
)

target_link_options(${ESP_TARGET_LIB}
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32s3.rom.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32s3.rom.api.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32s3.rom.libc.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32s3.rom.libgcc.ld
)
