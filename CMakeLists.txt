cmake_minimum_required(VERSION 3.14)

project(ambassador LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)
pkg_check_modules(SDL3_IMAGE REQUIRED IMPORTED_TARGET sdl3-image)

add_library(ambcore STATIC
    src/config.cxx
    src/ambassador.cxx
    src/event.cxx
    src/render.cxx
    src/loop.cxx
)

add_library(ambdata STATIC
    src/damb_loader.cxx
)

add_library(ambutility STATIC
    src/utility_string.cxx
    src/utility_parse.cxx
)

# Release Binary
add_executable(ambassador
    src/main.cxx
)

add_executable(dambassador
    src/dambassador_main.cxx
    src/dambassador.cxx
)

target_link_libraries(ambassador PRIVATE
    ambcore
    ambdata
    PkgConfig::SDL3
    PkgConfig::SDL3_IMAGE
)

set_target_properties(ambassador PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
)

target_link_libraries(dambassador PRIVATE
    ambutility
    ambdata
    PkgConfig::SDL3
    PkgConfig::SDL3_IMAGE
)

set_target_properties(dambassador PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
)
