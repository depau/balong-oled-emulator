#pragma once

#include <stdint.h>

typedef enum pixel_format {
  PIXEL_FORMAT_RGBA8888 = 0,
} pixel_format_t;

typedef enum image_format {
  IMAGE_FORMAT_RAW = 0,
} image_format_t;

typedef struct image_descriptor {
  size_t data_size;
  int width;
  int height;
  image_format_t image_format;
  pixel_format_t pixel_format;
  const uint8_t *data;
} image_descriptor_t;
