cmake_minimum_required(VERSION 3.10)
project(balong_oled_emulator)

set(CMAKE_CXX_STANDARD 23)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FC fontconfig REQUIRED)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

add_library(hooks SHARED
        emulator/src/hooks.cpp
)
target_include_directories(hooks PRIVATE include emulator/include)

add_executable(balong_oled_emulator
        emulator/src/main.cpp
        emulator/src/sdl_utils.cpp
        emulator/src/display.cpp
        emulator/src/timer.cpp
)

target_include_directories(balong_oled_emulator PRIVATE include emulator/include ${FC_INCLUDE_DIRS})
target_link_libraries(balong_oled_emulator PRIVATE
        SDL2::SDL2
        SDL2_ttf::SDL2_ttf
        ${FC_LIBRARIES}
        hooks
)
