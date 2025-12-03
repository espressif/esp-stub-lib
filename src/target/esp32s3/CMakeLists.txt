set(srcs
    src/flash.c
    src/trax.c
    src/uart.c
    src/usb_cdc.c
)

add_library(${ESP_TARGET_LIB} STATIC ${srcs})

target_link_options(${ESP_TARGET_LIB}
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32s3.rom.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32s3.rom.api.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32s3.rom.extra.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32s3.rom.libc.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32s3.rom.libgcc.ld
)
