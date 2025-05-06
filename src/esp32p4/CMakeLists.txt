set(srcs
    src/uart.c
    src/flash.c
)

add_library(${ESP_TARGET_LIB} STATIC ${srcs})

target_include_directories(${ESP_TARGET_LIB}
    PUBLIC include
)

target_link_options(${ESP_TARGET_LIB} PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32p4.rom.ld)
target_link_options(${ESP_TARGET_LIB} PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32p4.rom.libc.ld)
