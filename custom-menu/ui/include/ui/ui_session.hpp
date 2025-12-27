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
  uint32_t tick_timer_id = 0;
  int fps = 0;

  void tick(std::chrono::steady_clock::time_point now) const {
    assert(controller_api != nullptr && "Controller API is null");
    if (!screen_stack.empty()) {
      screen_stack.back()->tick(*controller_api, now);
    }
  }

  void ensure_tick_timer() {
    assert(controller_api != nullptr && "Controller API is null");
    const int new_fps = needs_ticks_per_second();

    // No change in fps, timer state is correct -> do nothing
    if (new_fps == fps && ((new_fps > 0 && tick_timer_id != 0) || (new_fps == 0 && tick_timer_id == 0)))
      return;

    // FPS changed, and we have a timer -> cancel it
    if (new_fps != fps && tick_timer_id != 0) {
      controller_api->cancel_timer(tick_timer_id);
      tick_timer_id = 0;
    }
    assert(tick_timer_id == 0);

    fps = new_fps;
    if (fps == 0)
      return;

    tick_timer_id = controller_api->schedule_timer(1000 / fps, true, [this] {
      tick(std::chrono::steady_clock::now());
      ensure_tick_timer();
    });
  }

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
   * Must be called when entering the UI session.
   */
  void on_enter() {
    render();
    ensure_tick_timer();
  }

  /**
   * Must be called when leaving the UI session.
   */
  void on_leave() {
    assert(controller_api != nullptr && "Controller API is null");
    if (tick_timer_id != 0) {
      controller_api->cancel_timer(tick_timer_id);
      tick_timer_id = 0;
    }
  }

  /**
   * Handle a keypress event by forwarding it to the top screen in the stack.
   *
   * @param button The button that was pressed
   */
  void handle_keypress(const int button) {
    assert(controller_api != nullptr && "Controller API is null");
    if (!screen_stack.empty()) {
      screen_stack.back()->handle_keypress(*controller_api, button);
    }
    ensure_tick_timer();
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
};
} // namespace ui
