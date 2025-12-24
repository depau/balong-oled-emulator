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
  void handle_keypress(int button) const;

  /**
   * Render the top screen in the stack.
   */
  void render() const;

  /**
   * Push a new screen onto the stack and render it.
   *
   * @param screen The screen to push
   */
  template<typename T>
  T &push_screen(std::unique_ptr<T> screen) {
    static_assert(std::derived_from<T, screens::iscreen> == true);
    screen_stack.push_back(std::move(screen));
    return *static_cast<T *>(screen_stack.back().get());
  }

  /**
   * Replace the top screen in the stack with a new screen and render it.
   *
   * @param screen The screen to replace with
   */
  template<typename T>
  T &replace_screen(std::unique_ptr<T> screen) {
    static_assert(std::derived_from<T, screens::iscreen> == true);
    if (!screen_stack.empty()) {
      screen_stack.pop_back();
    }
    return push_screen(std::move(screen));
  }

  /**
   * Pop the top screen from the stack and render the new top screen.
   * If the stack becomes empty, return to the main menu.
   */
  void pop_screen();

  /**
   * Get the number of ticks per second needed by the top screen in the stack.
   *
   * @return The number of ticks per second needed. Returns 0 if the stack is empty.
   */
  [[nodiscard]] int needs_ticks_per_second() const;

  /**
   * Call the tick function of the top screen in the stack.
   *
   * @param now The current time point, as a monotonic steady clock time point
   */
  void tick(std::chrono::steady_clock::time_point now) const;
};
} // namespace ui
