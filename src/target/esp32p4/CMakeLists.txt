set(srcs
    src/cache.c
    src/flash.c
    src/mmu.c
    src/slave_spi.c
    src/uart.c
    src/usb_serial_jtag.c
    src/usb_otg.c
    src/clock.c
    src/aes_xts.c
)

# The NAND bus calls ECO5 ROM GPIO functions. rev1 keeps the weak "not supported" defaults.
if(NOT (DEFINED ESP_TARGET_REV AND ESP_TARGET_REV STREQUAL "rev1"))
    list(APPEND srcs
        src/nand.c
        ../common/src/nand_gpspi.c
    )
endif()

add_library(${ESP_TARGET_LIB} STATIC ${srcs})

if(DEFINED ESP_TARGET_REV AND ESP_TARGET_REV STREQUAL "rev1")
    set(ROM_LD_FILE "esp32p4.rom.ld")
    set(ROM_LIBC_LD_FILE "esp32p4.rom.libc.ld")
    set(ROM_LIBGCC_LD_FILE "esp32p4.rom.libgcc.ld")
else()
    # Default ESP32-P4 uses ECO5 ROM files
    set(ROM_LD_FILE "esp32p4.rom.eco5.ld")
    set(ROM_LIBC_LD_FILE "esp32p4.rom.eco5.libc.ld")
    set(ROM_LIBGCC_LD_FILE "esp32p4.rom.eco5.libgcc.ld")
endif()

target_link_options(${ESP_TARGET_LIB}
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/${ROM_LD_FILE}
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32p4.rom.api.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp32p4.rom.extra.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/${ROM_LIBC_LD_FILE}
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/${ROM_LIBGCC_LD_FILE}
)
