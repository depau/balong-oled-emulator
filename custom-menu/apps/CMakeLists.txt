function(custom_menu_add_app order target)
    set(app_filename "${order}-${target}")

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
    )

    target_link_libraries(${target} PRIVATE ui)

    set_target_properties(${target} PROPERTIES
            PREFIX ""
            OUTPUT_NAME "${app_filename}"
    )
    setup_sanitizers(${target})
endfunction()

custom_menu_add_app(0 hello_world)
