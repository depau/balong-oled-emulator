add_library(ui SHARED
        src/clay.c
        src/image.cpp
        src/loading_screen.cpp
)
target_include_directories(ui PRIVATE
        "${COMMON_INCLUDE_DIR}"
        "${ASSETS_INCLUDE_DIR}"
        "${SYMBOLS_INCLUDE_DIR}"
        vendor
        include
)
add_dependencies(ui
        generate_asset_headers
)
apply_common_settings(ui)

install(TARGETS ui DESTINATION lib)
