#include <array>
#include <memory>

#include "assets/loading_spinner.h"
#include "ui/image.hpp"
#include "ui/screens/loading_screen.hpp"

using namespace ui::screens;

static std::array<std::unique_ptr<image_descriptor_t>, loading_screen::FRAMES> cached_frames;

static void ensure_frames_loaded() {
  if (cached_frames[0] != nullptr)
    return;
  constexpr int angle_step = 360 / loading_screen::FRAMES;
  for (int i = 0; i < loading_screen::FRAMES; ++i) {
    cached_frames[i] = ui::rotate_image(loading_spinner_image, i * angle_step, ui::rotation_boundary_mode::KEEP_SIZE);
  }
}

image_descriptor_t *loading_screen::get_current_frame() const {
  ensure_frames_loaded();
  return cached_frames[current_frame].get();
}
