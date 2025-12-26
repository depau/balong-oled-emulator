#pragma once
#include <chrono>
#include <memory>

#include "apps/app_api.hpp"
#include "screens/menu_screen.hpp"
#include "ui/screens/iscreen.hpp"

namespace ui {
class ui_session {
  display_controller_api *controller_api;
  std::vector<std::unique_ptr<screens::iscreen>> screen_stack;

public:
  ui_session() : controller_api(nullptr) {}
  explicit ui_session(display_controller_api &controller_api) : controller_api(&controller_api) {}

  // Forbid copying
  ui_session &operator=(const ui_session &) = delete;
  ui_session(const ui_session &) = delete;

  // Allow moving
  ui_session &operator=(ui_session &&) = default;
  ui_session(ui_session &&) = default;

  /**
   * Handle a keypress event by forwarding it to the top screen in the stack.
   *
   * @param button The button that was pressed
   */
  void handle_keypress(int button) const {
    assert(controller_api != nullptr && "Controller API is null");
    if (!screen_stack.empty()) {
      screen_stack.back()->handle_keypress(*controller_api, button);
    }
  }

  /**
   * Render the top screen in the stack.
   */
  void render() const {
    assert(controller_api != nullptr && "Controller API is null");
    if (!screen_stack.empty()) {
      screen_stack.back()->render(*controller_api);
    } else {
      controller_api->goto_main_menu();
    }
  }

  /**
   * Push a new screen onto the stack
   *
   * @param screen The screen to push
   */
  template<typename T>
  T &push_screen_norender(std::unique_ptr<T> screen) {
    static_assert(sdk19compat::derived_from<T, screens::iscreen> == true);
    screen_stack.push_back(std::move(screen));
    return *static_cast<T *>(screen_stack.back().get());
  }

  /**
   * Push a new screen onto the stack and render it.
   *
   * @param screen The screen to push
   */
  template<typename T>
  T &push_screen(std::unique_ptr<T> screen) {
    T &result = push_screen_norender(std::move(screen));
    render();
    return result;
  }

  /**
   * Replace the top screen in the stack with a new screen
   *
   * @param screen The screen to replace with
   */
  template<typename T>
  T &replace_screen_norender(std::unique_ptr<T> screen) {
    static_assert(sdk19compat::derived_from<T, screens::iscreen> == true);
    if (!screen_stack.empty()) {
      screen_stack.pop_back();
    }
    return push_screen(std::move(screen));
  }

  /**
   * Replace the top screen in the stack with a new screen and render it.
   *
   * @param screen The screen to replace with
   */
  template<typename T>
  T &replace_screen(std::unique_ptr<T> screen) {
    T &result = replace_screen_norender(std::move(screen));
    render();
    return result;
  }

  /**
   * Pop the top screen from the stack.
   */
  void pop_screen_norender() {
    assert(controller_api != nullptr && "Controller API is null");
    if (!screen_stack.empty()) {
      screen_stack.pop_back();
    }
  }

  /**
   * Pop the top screen from the stack and render the new top screen.
   * If the stack becomes empty, return to the main menu.
   */
  void pop_screen() {
    pop_screen_norender();
    render();
  }

  /**
   * Get the number of ticks per second needed by the top screen in the stack.
   *
   * @return The number of ticks per second needed. Returns 0 if the stack is empty.
   */
  [[nodiscard]] int needs_ticks_per_second() const {
    if (!screen_stack.empty()) {
      return screen_stack.back()->needs_ticks_per_second();
    }
    return 0;
  }

  /**
   * Call the tick function of the top screen in the stack.
   *
   * @param now The current time point, as a monotonic steady clock time point
   */
  void tick(std::chrono::steady_clock::time_point now) const {
    assert(controller_api != nullptr && "Controller API is null");
    if (!screen_stack.empty()) {
      screen_stack.back()->tick(*controller_api, now);
    }
  }
};
} // namespace ui
