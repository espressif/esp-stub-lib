set(common_srcs
    src/flash.c
)

add_library(${ESP_COMMON_LIB} STATIC ${common_srcs})

target_include_directories(${ESP_COMMON_LIB}
    PUBLIC include
)
