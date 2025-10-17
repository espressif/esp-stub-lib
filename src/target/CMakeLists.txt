# Parent CMakeLists for all target directories
# This file contains common setup for all targets

# Build base (interface headers)
add_subdirectory(base ${ESP_BASE_LIB})

# Build target-specific library
add_subdirectory(${ESP_TARGET} ${ESP_TARGET_LIB})

# Build common library
add_subdirectory(common ${ESP_COMMON_LIB})

# Apply common configurations to target library
# This ensures strong symbols override weak ones
target_link_options(${ESP_TARGET_LIB}
    PUBLIC -Wl,--whole-archive
)

# Set common include directories for target library
target_include_directories(${ESP_TARGET_LIB}
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}/${ESP_TARGET}/include
)

# Setup dependency chain: target → base, common → base + target
target_link_libraries(${ESP_TARGET_LIB} PUBLIC ${ESP_BASE_LIB})
target_link_libraries(${ESP_COMMON_LIB} PUBLIC ${ESP_BASE_LIB} ${ESP_TARGET_LIB})
