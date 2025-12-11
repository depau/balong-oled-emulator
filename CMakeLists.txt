cmake_minimum_required(VERSION 3.10)
project(balong_oled_emulator)

option(ENABLE_SANITIZERS "Enable sanitizers" OFF)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

set(CUSTOM_MENU_PLUGIN_BUILD_DIR "${CMAKE_BINARY_DIR}/plugins" CACHE PATH "")
set(CUSTOM_MENU_PLUGIN_INSTALL_DIR "bin/plugins" CACHE STRING "")

set(COMMON_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
message(STATUS "Common include directory: ${COMMON_INCLUDE_DIR}")

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

function(custom_menu_add_plugin order target)
    set(plugin_filename "${order}_${target}")

    file(GLOB_RECURSE PLUGIN_CXX_SOURCES CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/${target}/src/*.cpp"
    )
    file(GLOB_RECURSE PLUGIN_C_SOURCES CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/${target}/src/*.c"
    )

    add_library(${target} SHARED ${PLUGIN_CXX_SOURCES} ${PLUGIN_C_SOURCES})

    target_include_directories(${target} PRIVATE
            "${COMMON_INCLUDE_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/${target}/include"
            "${CMAKE_CURRENT_SOURCE_DIR}/vendor"
    )

    set_target_properties(${target} PROPERTIES
            PREFIX ""
            OUTPUT_NAME "${plugin_filename}"
            LIBRARY_OUTPUT_DIRECTORY "${CUSTOM_MENU_PLUGIN_BUILD_DIR}"
    )

    foreach (cfg Debug Release RelWithDebInfo MinSizeRel)
        string(TOUPPER "${cfg}" CFG_UPPER)
        set_target_properties(${target} PROPERTIES
                "LIBRARY_OUTPUT_DIRECTORY_${CFG_UPPER}"
                "${CUSTOM_MENU_PLUGIN_BUILD_DIR}/${cfg}"
        )
    endforeach ()

    install(TARGETS ${target}
            RUNTIME DESTINATION "${CUSTOM_MENU_PLUGIN_INSTALL_DIR}"
            LIBRARY DESTINATION "${CUSTOM_MENU_PLUGIN_INSTALL_DIR}"
    )

    setup_sanitizers(${target})
endfunction()

add_subdirectory(emulator)
add_subdirectory(custom-menu)
