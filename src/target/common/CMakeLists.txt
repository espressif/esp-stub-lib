set(common_srcs
    src/flash.c
    src/mem_utils.c
    src/uart.c
)

add_library(${ESP_COMMON_LIB} STATIC ${common_srcs})
