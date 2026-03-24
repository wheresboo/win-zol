cmake_minimum_required(VERSION 4.0)
project(zol LANGUAGES C CXX)

set(_third_party "${CMAKE_CURRENT_SOURCE_DIR}/third_party/")
file(GLOB_RECURSE _src "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

option(BUILD_SHARED_LIBS "Build shared libraries" ON)

add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
)

if (MSVC)
    add_compile_options(/std:c++latest)
else ()
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter -w -s -std=c++20)
endif ()

add_library(zol ${_src})
add_library(zol::zol ALIAS zol)

target_include_directories(
        zol PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
        ${_third_party}
)

set_target_properties(
        zol PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
)