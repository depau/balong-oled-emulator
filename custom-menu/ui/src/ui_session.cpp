#include <cassert>

#include "ui/ui_session.hpp"

void ui::ui_session::handle_keypress(const int button) const {
  assert(controller_api != nullptr && "Controller API is null");
  if (!screen_stack.empty()) {
    screen_stack.back()->handle_keypress(*controller_api, button);
  }
}

void ui::ui_session::render() const {
  assert(controller_api != nullptr && "Controller API is null");
  if (!screen_stack.empty()) {
    screen_stack.back()->render(*controller_api);
  } else {
    controller_api->goto_main_menu();
  }
}

void ui::ui_session::pop_screen() {
  assert(controller_api != nullptr && "Controller API is null");
  if (!screen_stack.empty()) {
    screen_stack.pop_back();
  }
}

int ui::ui_session::needs_ticks_per_second() const {
  if (!screen_stack.empty()) {
    return screen_stack.back()->needs_ticks_per_second();
  }
  return 0;
}

void ui::ui_session::tick(const std::chrono::steady_clock::time_point now) const {
  assert(controller_api != nullptr && "Controller API is null");
  if (!screen_stack.empty()) {
    screen_stack.back()->tick(*controller_api, now);
  }
}
