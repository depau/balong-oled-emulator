find_program(UV_EXECUTABLE uv)
if (NOT UV_EXECUTABLE)
    message(FATAL_ERROR "uv command not found. Please install uv (https://docs.astral.sh/uv/getting-started/installation/)")
endif ()

add_custom_command(
        OUTPUT "${ASSETS_INCLUDE_DIR}/assets/loading_spinner.h"
        COMMAND ${UV_EXECUTABLE} run python
        "${CMAKE_CURRENT_SOURCE_DIR}/convert_image_to_rgba.py"
        --variable-name loading_spinner_image
        "${CMAKE_CURRENT_SOURCE_DIR}/progress_activity_48dp_FFFFFF_FILL0_wght400_GRAD0_opsz48.png"
        "${ASSETS_INCLUDE_DIR}/assets/loading_spinner.h"
        DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/convert_image_to_rgba.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/progress_activity_48dp_FFFFFF_FILL0_wght400_GRAD0_opsz48.png"
        COMMENT "Generating spinner image header loading_spinner.h"
)

add_custom_target(generate_asset_headers ALL
        DEPENDS
        "${ASSETS_INCLUDE_DIR}/assets/loading_spinner.h"
)
