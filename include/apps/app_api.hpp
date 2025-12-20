#pragma once

#include <optional>
#include <span>
#include <utility>

#include "app_api.h"

namespace priv_api {

uint32_t app_api_schedule_stdfn_timer(app_api_t controller_api,
                                      uint32_t time,
                                      uint32_t repeat,
                                      std::function<void()> &&callback);

}

struct display_controller_api {
  /**
   * Get the current display mode
   *
   * @return The current display mode
   */
  [[nodiscard]] display_mode_t get_display_mode() const { return app_api_get_display_mode(this); }

  /**
   * Get the screen width in pixels
   *
   * @return The screen width in pixels
   */
  [[nodiscard]] size_t get_screen_width() const { return app_api_get_screen_width(this); }

  /**
   * Get the screen height in pixels
   *
   * @return The screen height in pixels
   */
  [[nodiscard]] size_t get_screen_height() const { return app_api_get_screen_height(this); }

  /**
   * Get a font ID by name and size
   *
   * @param font_name The name of the font
   * @param font_size The size of the font
   * @return The font ID, or FONT_NOT_FOUND if not found
   */
  [[nodiscard]] std::optional<uint16_t> get_font(const char *font_name, const int font_size) const {
    const uint16_t result = app_api_get_font(this, font_name, font_size);
    if (result == FONT_NOT_FOUND)
      return std::nullopt;
    return result;
  };

  /**
   * Render a frame using Clay rendering commands
   *
   * @param cmds The Clay rendering commands
   */
  void clay_render(const Clay_RenderCommandArray &cmds) { app_api_clay_render(this, &cmds); }

  /**
   * Draw a raw frame buffer to the screen
   *
   * @param buf The frame buffer data
   */
  void draw_frame(const std::span<uint16_t> &buf) { app_api_draw_frame(this, buf.data(), buf.size()); }

  /**
   * Draw a raw frame buffer to the screen
   *
   * @param buf The frame buffer data
   */
  void draw_frame(const std::span<const uint16_t> &buf) { app_api_draw_frame(this, buf.data(), buf.size()); }

  /**
   * Return to the main menu, leaving the current app. `on_leave` will be called before returning.
   */
  void goto_main_menu() { app_api_goto_main_menu(this); }

  /**
   * Report a fatal error to the controller.
   *
   * Renders an error message to the UI and exits the app, calling `on_leave`. If `unload_app` is true, the
   * app will be unloaded after reporting the error, calling its `on_teardown` function, and removing it from the
   * list of loaded apps.
   *
   * @param message The error message to display
   * @param unload_app Whether to unload the app after reporting the error
   */
  void fatal_error(const char *message, const bool unload_app) { app_api_fatal_error(this, message, unload_app); }

  /**
   * Register an app loader for a specific file extension
   *
   * @param file_extension The file extension to register (including the dot, e.g. ".so")
   * @param loader_fn The loader function to call when loading a app with the specified extension
   * @param userptr A user pointer to pass to the loader function
   */
  void register_app_loader(const char *file_extension, const app_loader_callback_fn_t loader_fn, void *userptr) {
    app_api_register_app_loader(this, file_extension, loader_fn, userptr);
  }

  /**
   * Register a timer callback
   *
   * @param time Time in milliseconds until the timer fires
   * @param repeat Whether the timer should repeat
   * @param callback The callback function to call when the timer fires
   * @param userptr A user pointer to pass to the callback function
   * @return The timer ID
   */
  uint32_t schedule_timer(uint32_t time, uint32_t repeat, void (*callback)(void *userptr), void *userptr) {
    return app_api_schedule_timer(this, time, repeat, callback, userptr);
  }

  /**
   * Register a timer callback
   *
   * @param time Time in milliseconds until the timer fires
   * @param repeat Whether the timer should repeat
   * @param callback The callback function to call when the timer fires
   * @return The timer ID
   */
  uint32_t schedule_timer(const uint32_t time, const uint32_t repeat, std::function<void()> &&callback) {
    return priv_api::app_api_schedule_stdfn_timer(this, time, repeat, std::move(callback));
  }

  /**
   * Cancel a previously registered timer
   *
   * @param timer_id The ID of the timer to cancel
   * @return 0 on success, non-zero on failure
   */
  uint32_t cancel_timer(uint32_t timer_id) { return app_api_cancel_timer(this, timer_id); }
};

template<typename T, typename Arg>
constexpr T *construct_maybe_with_arg(Arg &&arg) {
  if constexpr (requires { T{ std::forward<Arg>(arg) }; }) {
    // T(Arg) is well-formed, use it
    return new T{ std::forward<Arg>(arg) };
  } else {
    // Otherwise, fall back to default constructor
    return new T{};
  }
}

// The Android SDK 19 does not support std::same_as, so we reimplement it here
namespace sdk19compat {
template<typename _Tp, typename _Up>
concept __same_as = std::is_same_v<_Tp, _Up>;

template<typename _Tp, typename _Up>
concept same_as = __same_as<_Tp, _Up> && __same_as<_Up, _Tp>;
} // namespace sdk19compat

template<typename T>
concept HasOnEnter = requires(T &t, app_api_t controller_api) {
  { t.on_enter(controller_api) } -> sdk19compat::same_as<void>;
};

template<typename T>
concept HasOnLeave = requires(T &t, app_api_t controller_api) {
  { t.on_leave(controller_api) } -> sdk19compat::same_as<void>;
};

template<typename T>
concept HasOnKeypress = requires(T &t, app_api_t controller_api, int button) {
  { t.on_keypress(controller_api, button) } -> sdk19compat::same_as<void>;
};

template<typename T>
concept HasOnTeardown = requires(T &t, app_api_t controller_api) {
  { t.on_teardown(controller_api) } -> sdk19compat::same_as<void>;
};

template<typename T>
void on_enter_trampoline(void *userptr, app_api_t controller_api) {
  if constexpr (HasOnEnter<T>) {
    static_cast<T *>(userptr)->on_enter(controller_api);
  }
}

template<typename T>
void on_leave_trampoline(void *userptr, app_api_t controller_api) {
  if constexpr (HasOnLeave<T>) {
    static_cast<T *>(userptr)->on_leave(controller_api);
  }
}

template<typename T>
void on_keypress_trampoline(void *userptr, app_api_t controller_api, int button) {
  if constexpr (HasOnKeypress<T>) {
    static_cast<T *>(userptr)->on_keypress(controller_api, button);
  }
}

template<typename T>
void on_teardown_trampoline(void *userptr, app_api_t controller_api) {
  if constexpr (HasOnTeardown<T>) {
    static_cast<T *>(userptr)->on_teardown(controller_api);
  }
}

template<typename T>
consteval app_on_enter_fn_t get_on_enter_ptr() {
  if constexpr (HasOnEnter<T>) {
    return &on_enter_trampoline<T>;
  }
  return nullptr;
}

template<typename T>
consteval app_on_leave_fn_t get_on_leave_ptr() {
  if constexpr (HasOnLeave<T>) {
    return &on_leave_trampoline<T>;
  }
  return nullptr;
}

template<typename T>
consteval app_on_keypress_fn_t get_on_keypress_ptr() {
  if constexpr (HasOnKeypress<T>) {
    return &on_keypress_trampoline<T>;
  }
  return nullptr;
}

// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _DECLARE_CPP_APP_INTERNAL(_register_app_fn_name, _descriptor_name, _name, _class) \
  void *_##_class##__setup(app_api_t controller_api) {                                    \
    return construct_maybe_with_arg<_class>(controller_api);                              \
  }                                                                                       \
  void _##_class##__teardown(void *userptr, app_api_t controller_api) {                   \
    on_teardown_trampoline<_class>(userptr, controller_api);                              \
    delete reinterpret_cast<_class *>(userptr);                                           \
  }                                                                                       \
  _DECLARE_APP_INTERNAL(_register_app_fn_name,                                            \
                        _descriptor_name,                                                 \
                        _name,                                                            \
                        _##_class##__setup,                                               \
                        _##_class##__teardown,                                            \
                        get_on_enter_ptr<_class>(),                                       \
                        get_on_leave_ptr<_class>(),                                        \
                        get_on_keypress_ptr<_class>())

/**
 * Declare a C++ app.
 *
 * @snippet _app_example_snippet.cpp Demonstration of a C++ app using DECLARE_CPP_APP
 *
 * @param _name The name of the app
 * @param _class The app class name
 */
#define DECLARE_CPP_APP(_name, _class) _DECLARE_CPP_APP_INTERNAL(REGISTER_APP_FN, APP_DESCRIPTOR, _name, _class)
