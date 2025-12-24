find_program(UV_EXECUTABLE uv)
if (NOT UV_EXECUTABLE)
    message(FATAL_ERROR "uv command not found. Please install uv (https://docs.astral.sh/uv/getting-started/installation/)")
endif ()

add_custom_command(
        OUTPUT "${FONTS_INCLUDE_DIR}/fonts/poppins_12.hpp"
        COMMAND ${UV_EXECUTABLE} run python "${CMAKE_CURRENT_SOURCE_DIR}/generate_bitmap_font.py" "${CMAKE_CURRENT_SOURCE_DIR}/Poppins-Regular.ttf" 12 -o "${FONTS_INCLUDE_DIR}/fonts/poppins_12.hpp" --name Poppins --overrides "${CMAKE_CURRENT_SOURCE_DIR}/glyph_overrides.yaml"
        DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/generate_bitmap_font.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/Poppins-Regular.ttf"
        "${CMAKE_CURRENT_SOURCE_DIR}/MaterialSymbolsSharp_Filled-Regular.ttf"
        "${CMAKE_CURRENT_SOURCE_DIR}/glyph_overrides.yaml"
        COMMENT "Generating bitmap font header poppins_12.hpp"
)
add_custom_command(
        OUTPUT "${SYMBOLS_INCLUDE_DIR}/symbols.h"
        COMMAND ${UV_EXECUTABLE} run python "${CMAKE_CURRENT_SOURCE_DIR}/generate_glyph_header.py" "${CMAKE_CURRENT_SOURCE_DIR}/glyph_overrides.yaml" "${SYMBOLS_INCLUDE_DIR}/symbols.h"
        DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/generate_glyph_header.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/glyph_overrides.yaml"
        COMMENT "Generating symbols header symbols.h"
)

add_custom_target(generate_font_headers ALL
        DEPENDS
        "${FONTS_INCLUDE_DIR}/fonts/poppins_12.hpp"
        "${SYMBOLS_INCLUDE_DIR}/symbols.h"
)
