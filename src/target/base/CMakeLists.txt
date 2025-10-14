# Base layer: Interface library with headers only (no implementation)
add_library(${ESP_BASE_LIB} INTERFACE)

target_include_directories(${ESP_BASE_LIB}
    INTERFACE include
)
