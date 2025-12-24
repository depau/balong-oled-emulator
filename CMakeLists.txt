cmake_minimum_required(VERSION 3.10)
project(balong_oled_emulator)

option(ENABLE_SANITIZERS "Enable sanitizers" OFF)
option(EMULATOR "Build the emulator" ON)
option(DEBUG_MESSAGES "Enable debug messages" OFF)
option(DEBUG_RENDER_COMMANDS "Enable debug messages for Clay render commands" OFF)
option(DEBUG_HOST "Get debug messages from the oled executable when the custom menu library is loaded" OFF)
option(STRIP "Strip symbols from the final binaries to reduce size" ON)

set(CMAKE_CXX_STANDARD 23)

set(COMMON_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

function(apply_common_settings target)
    if (ENABLE_SANITIZERS)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            target_compile_options(${target} PRIVATE -fsanitize=address,undefined,leak,integer -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=address,undefined,leak,integer)
        else ()
            target_compile_options(${target} PRIVATE -fsanitize=address,undefined,leak -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=address,undefined,leak)
        endif ()
    endif ()
    if (STRIP)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND ${CMAKE_STRIP} --strip-unneeded $<TARGET_FILE:${target}>
                    COMMENT "Stripping symbols from ${target}"
            )
        else ()
            add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:${target}>
                    COMMENT "Stripping symbols from ${target}"
            )
        endif ()
    endif ()
    if (DEBUG_MESSAGES)
        target_compile_definitions(${target} PRIVATE DEBUG=1)
    endif ()
    if (DEBUG_RENDER_COMMANDS)
        target_compile_definitions(${target} PRIVATE DEBUG_RENDER_COMMANDS=1)
    endif ()
endfunction()

if (EMULATOR)
    find_package(PkgConfig)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules(FC fontconfig)
    endif ()

    find_package(SDL2)
    if (SDL2_FOUND)
        find_package(SDL2_ttf)
        if (NOT SDL2_ttf_FOUND)
            message(FATAL_ERROR "SDL2_ttf is required if SDL2 is found")
        endif ()
    endif ()

    if (NOT SDL2_FOUND OR NOT FC_FOUND)
        message(WARNING "SDL2 or Fontconfig not found; disabling emulator build.")
    else ()
        add_subdirectory(emulator)
    endif ()
endif ()

add_subdirectory(custom-menu)
