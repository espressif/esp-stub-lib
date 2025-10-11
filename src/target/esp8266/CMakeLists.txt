set(srcs
    src/mem_utils.c
    src/rom-missings.c
    src/rom-patched.c
)

add_library(${ESP_TARGET_LIB} STATIC ${srcs})

target_include_directories(${ESP_TARGET_LIB}
    PUBLIC include
)

target_link_options(${ESP_TARGET_LIB} PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp8266.rom.ld)
