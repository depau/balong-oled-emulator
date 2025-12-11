function(custom_menu_add_plugin order target)
    set(plugin_filename "${order}-${target}")

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
            "${CMAKE_CURRENT_SOURCE_DIR}/../vendor"
    )

    set_target_properties(${target} PROPERTIES
            PREFIX ""
            OUTPUT_NAME "${plugin_filename}"
    )
    setup_sanitizers(${target})
endfunction()

custom_menu_add_plugin(0 hello_world)
