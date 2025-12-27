set(ASSETS_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated_assets")
set(FONTS_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated_fonts")
set(SYMBOLS_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated_symbols")
file(MAKE_DIRECTORY "${ASSETS_INCLUDE_DIR}/assets")
file(MAKE_DIRECTORY "${FONTS_INCLUDE_DIR}/fonts")
file(MAKE_DIRECTORY "${SYMBOLS_INCLUDE_DIR}")

add_subdirectory(assets)
add_subdirectory(fonts)

add_library(balong_custom_menu SHARED
        src/main.cpp
        src/display_controller.cpp
        src/app_api.cpp
        src/main_menu.cpp
        src/so_app_loader.cpp
)
target_include_directories(balong_custom_menu PRIVATE
        "${COMMON_INCLUDE_DIR}"
        "${ASSETS_INCLUDE_DIR}"
        "${FONTS_INCLUDE_DIR}"
        "${SYMBOLS_INCLUDE_DIR}"
        include
        app-api/include
        ui/vendor
        ui/include
)
target_link_libraries(balong_custom_menu PRIVATE
        ui
)
target_compile_options(balong_custom_menu PRIVATE -fvisibility=hidden -fvisibility-inlines-hidden)
target_link_options(balong_custom_menu PRIVATE "-Wl,--exclude-libs,ALL")
target_compile_definitions(balong_custom_menu PRIVATE INSTALL_PREFIX=\"${CMAKE_INSTALL_PREFIX}\")
apply_common_settings(balong_custom_menu)
add_dependencies(
        generate_asset_headers
        generate_font_headers
)

if (DEBUG_HOST)
    target_compile_definitions(balong_custom_menu PRIVATE DEBUG_HOST=1)
endif ()

install(TARGETS balong_custom_menu DESTINATION lib)

add_subdirectory(ui)
add_subdirectory(apps)
