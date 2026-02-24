cmake_minimum_required(VERSION 3.14)

project(ambassador LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(zstd REQUIRED)

add_library(libcore STATIC
    src/config.cxx
    src/ambassador.cxx
    src/event.cxx
    src/render.cxx
    src/loop.cxx
)

add_library(libdata STATIC
    src/scenedata.cxx
)

# Release Binary
add_executable(ambassador
    src/main.cxx
)

add_executable(dambassador
    src/dambassador.cxx
)

target_link_libraries(ambassador PRIVATE
    libcore
    libdata
    SDL3::SDL3
    SDL3_image::SDL3_image
    zstd::libzstd_shared
)

set_target_properties(ambassador PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
)

target_link_libraries(dambassador PRIVATE
    libdata
    SDL3::SDL3
    SDL3_image::SDL3_image
    zstd::libzstd_shared
)

set_target_properties(dambassador PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
)
