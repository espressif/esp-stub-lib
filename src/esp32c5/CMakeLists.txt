set(srcs
    src/uart.c
    src/flash.c
)

add_library(${ESP_TARGET_LIB} STATIC ${srcs})

target_include_directories(${ESP_TARGET_LIB}
    PUBLIC include
)

target_link_options(${ESP_TARGET_LIB} PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32c5.rom.ld)
target_link_options(${ESP_TARGET_LIB} PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32c5.rom.libc.ld)
