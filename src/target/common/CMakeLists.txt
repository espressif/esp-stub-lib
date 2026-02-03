set(common_srcs
    src/aes_xts.c
    src/flash.c
    src/flash_4byte.c
    src/mem_utils.c
    src/security.c
    src/uart.c
    src/usb_serial_jtag.c
    src/usb_otg.c
    src/md5.c
)

add_library(${ESP_COMMON_LIB} STATIC ${common_srcs})
