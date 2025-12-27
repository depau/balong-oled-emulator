#!/usr/bin/env python
import argparse
import subprocess

from PIL import Image


def image_to_c_header(image_path, header_path, variable_name):
    """
    Converts an image to a C header file with RGBA raw data.

    :param image_path: Path to the input image.
    :param header_path: Path to the output header file.
    :param variable_name: The name for the C variable.
    """
    try:
        # Open the image and convert to RGBA
        img = Image.open(image_path).convert("RGBA")
        width, height = img.size
        raw_data = img.tobytes()

        # Build the header content
        content = "#pragma once\n\n"
        content += '#include <stdint.h>\n\n'
        content += '#include "image_descriptor.h"\n\n'

        content += f"inline const uint8_t {variable_name}_data[] = {{    "
        for i, byte in enumerate(raw_data):
            content += "0x{:02x}, ".format(byte)  # noqa: W605
            if (i + 1) % 16 == 0:
                content += "\n    "
        content += "\n};\n\n"

        content += f"inline const image_descriptor_t {variable_name} = {{ \n"
        content += f"    .data_size = sizeof({variable_name}_data),\n"
        content += f"    .width = {width},\n"
        content += f"    .height = {height},\n"
        content += f"    .image_format = IMAGE_FORMAT_RAW,\n"
        content += f"    .pixel_format = PIXEL_FORMAT_RGBA8888,\n"
        content += f"    .data = {variable_name}_data\n"
        content += "};\n"

        with open(header_path, "w") as f:
            f.write(content)

        subprocess.run(["clang-format", "-i", header_path], check=True)

    except Exception as e:
        print(f"An error occurred: {e}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Convert an image to a C header file.")
    parser.add_argument("input_image", help="Path to the input image file.")
    parser.add_argument("output_header", help="Path to the output C header file.")
    parser.add_argument("--variable-name", help="Name for the C variable.", default="image")
    args = parser.parse_args()

    image_to_c_header(args.input_image, args.output_header, args.variable_name)
