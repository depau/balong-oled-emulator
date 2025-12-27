#pragma once

#include <algorithm>
#include <assert.h>
#include <cmath>
#include <cstdint>
#include <memory>

#include "clay.hpp"
#include "image_descriptor.h"

#define UI_CLAY_IMAGE_DATA(_desc) ((void *) (&(_desc)))
#define UI_CLAY_IMAGE_SIZING(_desc) \
  { .width = CLAY_SIZING_FIXED((float) (_desc).width), .height = CLAY_SIZING_FIXED((float) (_desc).height) }

namespace ui {

struct rgba_pixel {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
};

class image_with_data : public image_descriptor_t {
  std::unique_ptr<rgba_pixel[]> owned_data;

public:
  image_with_data(const int width,
                  const int height,
                  const image_format_t image_format,
                  const pixel_format_t pixel_format) {
    assert(image_format == IMAGE_FORMAT_RAW);
    assert(pixel_format == PIXEL_FORMAT_RGBA8888);
    this->width = width;
    this->height = height;
    this->image_format = image_format;
    this->pixel_format = pixel_format;
    this->data_size = static_cast<size_t>(width * height) * 4;
    owned_data = std::make_unique<rgba_pixel[]>(static_cast<size_t>(width * height));
    this->data = reinterpret_cast<uint8_t *>(owned_data.get());
  }

  image_with_data &operator=(const image_with_data &other) {
    if (this != &other) {
      width = other.width;
      height = other.height;
      image_format = other.image_format;
      pixel_format = other.pixel_format;
      data_size = other.data_size;
      owned_data = std::make_unique<rgba_pixel[]>(static_cast<size_t>(width * height));
      std::copy_n(other.owned_data.get(), static_cast<size_t>(width * height), owned_data.get());
      data = reinterpret_cast<const uint8_t *>(owned_data.get());
    }
    return *this;
  }

  image_with_data(const image_with_data &other) { *this = other; }

  image_with_data &operator=(image_with_data &&other) noexcept {
    if (this != &other) {
      width = other.width;
      height = other.height;
      image_format = other.image_format;
      pixel_format = other.pixel_format;
      data_size = other.data_size;
      owned_data = std::move(other.owned_data);
      data = reinterpret_cast<const uint8_t *>(owned_data.get());
    }
    return *this;
  }

  image_with_data(image_with_data &&other) noexcept { *this = std::move(other); }

  size_t size() const { return static_cast<size_t>(width) * static_cast<size_t>(height); }

  rgba_pixel *get_data() const { return owned_data.get(); }

  rgba_pixel &operator[](const size_t index) const { return get_data()[index]; }
};

enum class rotation_boundary_mode {
  /**
   * The rotated image will have the same size as the source image.
   * Parts of the image that fall outside the original dimensions will be clipped.
   */
  KEEP_SIZE,

  /**
   * The rotated image will be large enough to fit the entire rotated content.
   * There may be empty areas near the corners.
   */
  EXPAND_SIZE,
};

/**
 * Rotate an image by the specified angle (in degrees).
 * If the angle is a multiple of 90, the image is rotated by simple pixel transposition.
 * Otherwise, the image is rotated by inverse mapping with nearest-neighbor interpolation.
 *
 * @param src Source image descriptor
 * @param angle_deg Rotation angle in degrees (positive values rotate clockwise)
 * @param mode Boundary mode for handling image boundaries
 * @return A unique pointer to the rotated image descriptor
 */
std::unique_ptr<image_descriptor_t>
rotate_image(const image_descriptor_t &src,
             const int angle_deg,
             const rotation_boundary_mode mode = rotation_boundary_mode::EXPAND_SIZE);

}; // namespace ui