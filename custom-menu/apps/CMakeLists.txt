function(custom_menu_add_app option order target)
    set(app_filename "${order}-${target}")

    if (option)
        if (NOT ${option})
            message(STATUS "Skipping build of app: ${target}")
            return()
        endif ()
    endif ()

    file(GLOB_RECURSE APP_CXX_SOURCES CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/${target}/src/*.cpp"
    )
    file(GLOB_RECURSE APP_C_SOURCES CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/${target}/src/*.c"
    )

    add_library(${target} SHARED ${APP_CXX_SOURCES} ${APP_C_SOURCES})

    target_include_directories(${target} PRIVATE
            "${COMMON_INCLUDE_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/${target}/include"
            "${CMAKE_CURRENT_SOURCE_DIR}/../ui/vendor"
            "${CMAKE_CURRENT_SOURCE_DIR}/../ui/include"
            "${SYMBOLS_INCLUDE_DIR}"
    )

    target_link_libraries(${target} PRIVATE ui balong_custom_menu)

    set_target_properties(${target} PROPERTIES
            PREFIX ""
            OUTPUT_NAME "${app_filename}"
    )
    apply_common_settings(${target})

    add_dependencies(
            ${target}
            generate_font_headers
    )

    install(TARGETS ${target} DESTINATION apps)
endfunction()

option(APPS_HELLO_WORLD "Build Hello World App" ON)
set(APPS_HELLO_WORLD_ORDER 0 CACHE STRING "Order of Hello World App in Menu")

option(APPS_MATRIX "Build Matrix App" ON)
set(APPS_MATRIX_ORDER 1 CACHE STRING "Order of Matrix App in Menu")

custom_menu_add_app(APPS_HELLO_WORLD ${APPS_HELLO_WORLD_ORDER} hello_world)
custom_menu_add_app(APPS_MATRIX ${APPS_MATRIX_ORDER} matrix)
