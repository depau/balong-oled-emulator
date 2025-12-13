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
setup_sanitizers(balong_custom_menu)

add_subdirectory(ui)
add_subdirectory(apps)
