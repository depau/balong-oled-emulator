#include <cassert>
#include <memory>

#include "debug.h"
#include "ui/image.hpp"

using namespace ui;

std::unique_ptr<image_descriptor_t>
ui::rotate_image(const image_descriptor_t &src, const int angle_deg, const rotation_boundary_mode mode) {
  if (angle_deg % 360 == 0) {
    return std::make_unique<image_descriptor_t>(src);
  }

  if (src.image_format != IMAGE_FORMAT_RAW || src.pixel_format != PIXEL_FORMAT_RGBA8888) {
    debugf("rotate_image: Unsupported image format or pixel format\n");
    return std::make_unique<image_descriptor_t>(src);
  }

  int angle = angle_deg % 360;
  if (angle < 0)
    angle += 360;

  int dest_width = 0;
  int dest_height = 0;

  if (angle == 90 || angle == 270) {
    dest_width = src.height;
    dest_height = src.width;
  } else if (angle == 180) {
    dest_width = src.width;
    dest_height = src.height;
  } else if (mode == rotation_boundary_mode::KEEP_SIZE) {
    dest_width = src.width;
    dest_height = src.height;
  } else {
    const double radians = static_cast<double>(angle) * M_PI / 180.0;
    const double cos_angle = std::abs(std::cos(radians));
    const double sin_angle = std::abs(std::sin(radians));
    dest_width = static_cast<int>(std::ceil(src.width * cos_angle + src.height * sin_angle));
    dest_height = static_cast<int>(std::ceil(src.width * sin_angle + src.height * cos_angle));
  }

  auto dst = std::make_unique<image_with_data>(dest_width, dest_height, src.image_format, src.pixel_format);

  if (angle % 90 == 0) {
    for (int y = 0; y < src.height; ++y) {
      for (int x = 0; x < src.width; ++x) {
        const size_t src_idx = (static_cast<size_t>(y) * static_cast<size_t>(src.width) + static_cast<size_t>(x)) * 4;
        size_t dst_x, dst_y;

        switch (angle) {
        case 90:
          dst_x = src.height - 1 - y;
          dst_y = x;
          break;
        case 180:
          dst_x = src.width - 1 - x;
          dst_y = src.height - 1 - y;
          break;
        case 270:
          dst_x = y;
          dst_y = src.width - 1 - x;
          break;
        default:
          assert(false && "Unhandled angle");
        }

        const size_t dst_idx = dst_y * static_cast<size_t>(dest_width) + dst_x;

        (*dst)[dst_idx].r = src.data[src_idx + 0];
        (*dst)[dst_idx].g = src.data[src_idx + 1];
        (*dst)[dst_idx].b = src.data[src_idx + 2];
        (*dst)[dst_idx].a = src.data[src_idx + 3];
      }
    }
  } else {
    const double radians = static_cast<double>(angle) * M_PI / 180.0;
    const double cos_angle = std::cos(radians);
    const double sin_angle = std::sin(radians);

    const double src_cx = static_cast<double>(src.width) / 2.0;
    const double src_cy = static_cast<double>(src.height) / 2.0;
    const double dst_cx = static_cast<double>(dest_width) / 2.0;
    const double dst_cy = static_cast<double>(dest_height) / 2.0;

    for (int y = 0; y < dest_height; ++y) {
      for (int x = 0; x < dest_width; ++x) {
        const double dx = static_cast<double>(x) - dst_cx;
        const double dy = static_cast<double>(y) - dst_cy;

        const double src_x_f = cos_angle * dx + sin_angle * dy + src_cx;
        const double src_y_f = -sin_angle * dx + cos_angle * dy + src_cy;

        const int src_x = static_cast<int>(std::round(src_x_f));
        const int src_y = static_cast<int>(std::round(src_y_f));

        if (src_x >= 0 && src_x < src.width && src_y >= 0 && src_y < src.height) {
          const size_t src_idx = (static_cast<size_t>(src_y) * static_cast<size_t>(src.width) +
                                  static_cast<size_t>(src_x)) *
                                 4;
          const size_t dst_idx = static_cast<size_t>(y) * static_cast<size_t>(dest_width) + static_cast<size_t>(x);

          (*dst)[dst_idx].r = src.data[src_idx + 0];
          (*dst)[dst_idx].g = src.data[src_idx + 1];
          (*dst)[dst_idx].b = src.data[src_idx + 2];
          (*dst)[dst_idx].a = src.data[src_idx + 3];
        }
      }
    }
  }

  return dst;
}
