add_library(balong_custom_menu SHARED
        src/main.cpp
        src/display_controller.cpp
        src/app_api.cpp
        src/main_menu.cpp
        src/so_app_loader.cpp
)
target_include_directories(balong_custom_menu PRIVATE
        "${COMMON_INCLUDE_DIR}"
        include
        ui/vendor
        ui/include
)
target_link_libraries(balong_custom_menu PRIVATE
        ui
)
target_compile_options(balong_custom_menu PRIVATE -fvisibility=hidden -fvisibility-inlines-hidden)
target_link_options(balong_custom_menu PRIVATE "-Wl,--exclude-libs,ALL")
apply_common_settings(balong_custom_menu)

if (DEBUG_HACK)
    target_compile_definitions(balong_custom_menu PRIVATE DEBUG_HACK=1)
endif ()

add_subdirectory(ui)
add_subdirectory(apps)
