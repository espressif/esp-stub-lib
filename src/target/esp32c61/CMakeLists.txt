set(srcs
    src/uart.c
    src/usb_serial_jtag.c
    src/clock.c
)

add_library(${ESP_TARGET_LIB} STATIC ${srcs})

target_link_options(${ESP_TARGET_LIB}
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32c61.rom.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32c61.rom.api.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32c61.rom.libc.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32c61.rom.libgcc.ld
)
