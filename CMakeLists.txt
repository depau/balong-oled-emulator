cmake_minimum_required(VERSION 3.10)
project(balong_oled_emulator)

option(ENABLE_SANITIZERS "Enable sanitizers" OFF)

set(CMAKE_CXX_STANDARD 23)

set(COMMON_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

find_package(PkgConfig REQUIRED)
pkg_check_modules(FC fontconfig REQUIRED)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

function(setup_sanitizers target)
    if (ENABLE_SANITIZERS)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            target_compile_options(${target} PRIVATE -fsanitize=address,undefined,leak,integer -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=address,undefined,leak,integer)
        else ()
            target_compile_options(${target} PRIVATE -fsanitize=address,undefined,leak -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=address,undefined,leak)
        endif ()
    endif ()
endfunction()

add_subdirectory(emulator)
add_subdirectory(custom-menu)
