add_library(ui SHARED
        src/clay.c
        src/ui_session.cpp
)
target_include_directories(ui PRIVATE
        "${COMMON_INCLUDE_DIR}"
        vendor
        include
)
apply_common_settings(ui)
