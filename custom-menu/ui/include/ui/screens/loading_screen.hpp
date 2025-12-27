#pragma once

#include <chrono>
#include <functional>

#include "iscreen.hpp"
#include "ui/image.hpp"
#include "ui/utils.hpp"

namespace ui::screens {

class loading_screen final : public iscreen {
  std::function<void(int)> on_keypress;
  size_t current_frame = 0;

  image_descriptor_t *get_current_frame() const;

public:
  static constexpr int FRAMES = 18;
  static constexpr int FPS = FRAMES * 2;

  loading_screen() = default;
  explicit loading_screen(std::function<void(int)> &&on_keypress) : on_keypress(std::move(on_keypress)) {}

  void render(display_controller_api &controller_api) override {
    Clay_BeginLayout();
    CLAY(CLAY_ID("Root"), {
      .layout = {
        .sizing = ROOT_ELEMENT_SIZING(&controller_api),
        .padding = CLAY_PADDING_ALL(ROOT_PADDING),
        .childGap = ROOT_PADDING,
        .childAlignment = {
          .x = CLAY_ALIGN_X_CENTER,
          .y = CLAY_ALIGN_Y_CENTER,
        },
        .layoutDirection = CLAY_TOP_TO_BOTTOM,
      },
      .backgroundColor = ui::theme::COLOR_SURFACE,
    }) {
      const auto *frame = get_current_frame();
      CLAY(CLAY_ID("LoadingSpinner"), {
        .layout = {
          .sizing = UI_CLAY_IMAGE_SIZING(*frame),
        },
        .image = {
          .imageData = UI_CLAY_IMAGE_DATA(*frame),
        }
      });
    }
    controller_api.clay_render(Clay_EndLayout());
  }

  [[nodiscard]] int needs_ticks_per_second() const override { return FPS; }

  void tick(display_controller_api &controller_api, std::chrono::steady_clock::time_point now) override {
    current_frame = (current_frame + 1) % FRAMES;
    render(controller_api);
  }

  void handle_keypress(display_controller_api &controller_api, const int button) override {
    if (on_keypress) {
      on_keypress(button);
    }
  }
};
} // namespace ui::screens