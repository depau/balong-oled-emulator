#pragma once
#include <chrono>
#include <memory>

#include "Screen.hpp"
#include "ScreenStack.hpp"
#include "apps/app_api.hpp"

namespace ui {
class UISession {
  display_controller_api &controller_api;
  ScreenStack stack;

public:
  explicit UISession(display_controller_api &controller_api);

  void handle_keypress(int button);

  void set_root_screen(std::unique_ptr<Screen> screen);

  [[nodiscard]] int needsTicksPerSecond();

  void tick(std::chrono::steady_clock::time_point now);
};
} // namespace ui