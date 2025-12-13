add_library(ui SHARED
        src/clay.c
        src/UISession.cpp
        src/UIContext.cpp
        src/ScreenStack.cpp
        src/Screen.cpp
)
target_include_directories(ui PRIVATE
        "${COMMON_INCLUDE_DIR}"
        vendor
        include
)
setup_sanitizers(ui)
