set(common_srcs
    src/flash.c
    src/mem_utils.c
    src/security.c
    src/uart.c
    src/usb_cdc.c
    src/usb_cdc_common.c
)

add_library(${ESP_COMMON_LIB} STATIC ${common_srcs})
