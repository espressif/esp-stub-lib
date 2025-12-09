set(srcs
    src/uart.c
    src/usb_serial_jtag.c
)

add_library(${ESP_TARGET_LIB} STATIC ${srcs})

target_link_options(${ESP_TARGET_LIB}
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32h2.rom.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32h2.rom.api.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32h2.rom.libc.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32h2.rom.libgcc.ld
)
