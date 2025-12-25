add_library(hooks SHARED
        src/hooks.cpp
)
target_include_directories(hooks PRIVATE
        "${COMMON_INCLUDE_DIR}"
        include
)
apply_common_settings(hooks)

add_executable(balong_oled_emulator
        src/main.cpp
        src/sdl_utils.cpp
        src/display.cpp
        src/timer.cpp
)
target_include_directories(balong_oled_emulator PRIVATE
        "${COMMON_INCLUDE_DIR}"
        include
        ${FC_INCLUDE_DIRS}
)
target_link_libraries(balong_oled_emulator PRIVATE
        SDL2::SDL2
        SDL2_ttf::SDL2_ttf
        ${FC_LIBRARIES}
        hooks
)
apply_common_settings(balong_oled_emulator)

install(TARGETS balong_oled_emulator DESTINATION bin)
install(TARGETS hooks DESTINATION lib)
