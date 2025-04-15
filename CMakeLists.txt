if(NOT DEFINED ESP_TARGET)
    message(FATAL_ERROR "ESP_TARGET not defined. Please specify -DESP_TARGET=<target>")
endif()

if(NOT ESP_STUB_LIB)
    set(ESP_STUB_LIB esp-stub-lib)
endif()

if(NOT ESP_TARGET_LIB)
    set(ESP_TARGET_LIB ${ESP_TARGET})
endif()

set(srcs
    src/log.c
    src/flash.c
)

add_library(${ESP_STUB_LIB} STATIC ${srcs})

target_include_directories(${ESP_STUB_LIB}
    PUBLIC include
    PRIVATE include/esp-stub-lib
)

# STUB_COMPILE_DEFS is optional definitions coming from the parent CMakeLists.txt
target_compile_definitions(${ESP_STUB_LIB} PRIVATE ${STUB_COMPILE_DEFS})

add_subdirectory(src/${ESP_TARGET} ${ESP_TARGET_LIB})

target_link_libraries(${ESP_STUB_LIB} PUBLIC ${ESP_TARGET_LIB})
