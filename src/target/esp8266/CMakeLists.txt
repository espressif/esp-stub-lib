set(srcs
    src/uart.c
    src/rom-patched.c
    src/security.c
    src/flash.c
    src/clock.c
    src/sha.c
)

add_library(${ESP_TARGET_LIB} STATIC ${srcs})

add_library(miniz_obj OBJECT src/miniz.c)

target_include_directories(miniz_obj PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/esp-stub-lib
)

target_compile_options(miniz_obj PRIVATE
    # Disable specific warnings that miniz cannot satisfy
    -Wno-conversion         # Disable implicit conversion warnings
    -Wno-sign-conversion    # Disable signed/unsigned conversion warnings
)

target_sources(${ESP_TARGET_LIB} PRIVATE $<TARGET_OBJECTS:miniz_obj>)

target_link_options(${ESP_TARGET_LIB}
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp8266.rom.ld
    PUBLIC -T${CMAKE_CURRENT_LIST_DIR}/ld/esp8266.rom.api.ld)
