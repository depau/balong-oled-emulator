cmake_minimum_required(VERSION 3.10)
project(balong_oled_emulator)

set(CMAKE_CXX_STANDARD 23)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FC fontconfig REQUIRED)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

add_library(hooks SHARED
        src/hooks.cpp
)
target_include_directories(hooks PRIVATE include)

add_executable(balong_oled_emulator
        src/main.cpp
        src/sdl_utils.cpp
        src/display.cpp
        src/timer.cpp
)

target_include_directories(balong_oled_emulator PRIVATE include ${FC_INCLUDE_DIRS})
target_link_libraries(balong_oled_emulator PRIVATE
        SDL2::SDL2
        SDL2_ttf::SDL2_ttf
        ${FC_LIBRARIES}
        hooks
)
