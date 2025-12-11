add_library(balong_custom_menu SHARED
        src/main.cpp
        src/display_controller.cpp
        src/clay.c
        src/plugin_api.cpp
        src/main_menu.cpp
        plugins/hello_world/src/plugin.cpp
)
target_include_directories(balong_custom_menu PRIVATE
        "${COMMON_INCLUDE_DIR}"
        vendor
        include
)
setup_sanitizers(balong_custom_menu)

add_subdirectory(plugins)
