cmake_minimum_required(VERSION 3.10)
project(balong_oled_emulator)

option(ENABLE_SANITIZERS "Enable sanitizers" OFF)

set(CMAKE_CXX_STANDARD 23)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FC fontconfig REQUIRED)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

add_library(hooks SHARED
        emulator/src/hooks.cpp
)
target_include_directories(hooks PRIVATE
        include
        emulator/include
)

add_executable(balong_oled_emulator
        emulator/src/main.cpp
        emulator/src/sdl_utils.cpp
        emulator/src/display.cpp
        emulator/src/timer.cpp
)
target_include_directories(balong_oled_emulator PRIVATE
        include
        emulator/include
        ${FC_INCLUDE_DIRS}
)
target_link_libraries(balong_oled_emulator PRIVATE
        SDL2::SDL2
        SDL2_ttf::SDL2_ttf
        ${FC_LIBRARIES}
        hooks
)

add_library(balong_custom_menu SHARED
        custom-menu/src/main.cpp
        custom-menu/src/display_controller.cpp
        custom-menu/src/clay.c
        custom-menu/src/plugin_api.cpp
        custom-menu/src/main_menu.cpp
        custom-menu/src/main_menu.hpp
)
target_include_directories(balong_custom_menu PRIVATE
        include
        custom-menu/vendor
        custom-menu/include
)

set(TARGETS hooks balong_oled_emulator balong_custom_menu)
foreach (target ${TARGETS})
    if (ENABLE_SANITIZERS)
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined,leak,integer -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined,leak,integer)
    endif ()
endforeach ()
