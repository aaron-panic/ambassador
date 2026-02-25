cmake_minimum_required(VERSION 3.14)

project(ambassador LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)
pkg_check_modules(SDL3_IMAGE REQUIRED IMPORTED_TARGET sdl3-image)

set(AMBCORE_HEADERS
    src/ambassador.hxx
    src/amb_types.hxx
    src/config.hxx
)

set(AMBCORE_SOURCES
    src/ambassador.cxx
    src/config.cxx
    src/event.cxx
    src/loop.cxx
    src/render.cxx
)

set(AMBUTILITY_HEADERS
    src/utility_binary.hxx
    src/utility_parse.hxx
    src/utility_string.hxx
)

set(AMBUTILITY_SOURCES
    src/utility_parse.cxx
    src/utility_string.cxx
)

set(AMBDATA_HEADERS
    src/damb_loader.hxx
    src/damb_spec.hxx
    src/format_damb.hxx
    src/runtime_atlas.hxx
    src/runtime_damb.hxx
    src/runtime_image.hxx
    src/runtime_map.hxx
    src/runtime_object.hxx
    src/visual_layers.hxx
)

set(AMBDATA_SOURCES
    src/damb_loader.cxx
)

add_library(ambcore STATIC)
target_sources(ambcore PRIVATE ${AMBCORE_SOURCES} ${AMBCORE_HEADERS})
target_include_directories(ambcore PUBLIC src)

add_library(ambutility STATIC)
target_sources(ambutility PRIVATE ${AMBUTILITY_SOURCES} ${AMBUTILITY_HEADERS})
target_include_directories(ambutility PUBLIC src)

add_library(ambdata STATIC)
target_sources(ambdata PRIVATE ${AMBDATA_SOURCES} ${AMBDATA_HEADERS})
target_include_directories(ambdata PUBLIC src)
target_link_libraries(ambdata PUBLIC ambutility PkgConfig::SDL3 PkgConfig::SDL3_IMAGE)

add_executable(ambassador src/main.cxx)
target_link_libraries(ambassador PRIVATE ambcore ambdata PkgConfig::SDL3 PkgConfig::SDL3_IMAGE)
set_target_properties(ambassador PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test")

add_executable(dambassador
    src/dambassador.cxx
    src/dambassador.hxx
    src/dambassador_main.cxx
)
target_link_libraries(dambassador PRIVATE ambutility ambdata PkgConfig::SDL3 PkgConfig::SDL3_IMAGE)
set_target_properties(dambassador PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test")
