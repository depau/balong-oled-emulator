add_library(ui SHARED
        src/clay.c
        src/ui_session.cpp
        src/ui_context.cpp
        src/screen_stack.cpp
        src/screen.cpp
)
target_include_directories(ui PRIVATE
        "${COMMON_INCLUDE_DIR}"
        vendor
        include
)
setup_sanitizers(ui)
