#pragma once

#include <algorithm>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "app_api.h"
#include "sdk19compat.hpp"
#include "template_utils.hpp"

namespace priv_api {

EXPORT uint32_t app_api_schedule_stdfn_timer(app_api_t controller_api,
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
  void fatal_error(const char *message, const bool unload_app = false) {
    app_api_fatal_error(this, message, unload_app);
  }

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
  void fatal_error(const std::string &message, const bool unload_app = false) {
    app_api_fatal_error(this, message.c_str(), unload_app);
  }

  /**
   * Register an app loader for a specific file extension
   *
   * @param file_extension The file extension to register (including the dot, e.g. ".so")
   * @param loader_fn The loader function to call when loading an app with the specified extension
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
  uint32_t schedule_timer(const uint32_t time, const uint32_t repeat, void (*callback)(void *userptr), void *userptr) {
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

  /**
   * Measure the dimensions of a text string with the given configuration, using Clay's text measurement.
   *
   * @param text The text string to measure
   * @param length The length of the text string
   * @param config The text element configuration
   * @return The measured dimensions
   */
  measure_text_result_t clay_measure_text(const char *text, const size_t length, Clay_TextElementConfig *config) {
    return app_api_clay_measure_text(this, text, length, config);
  }

  /**
   * Measure the dimensions of a text string with the given configuration, using Clay's text measurement.
   *
   * @param text The text string to measure
   * @param config The text element configuration
   * @return The measured dimensions
   */
  measure_text_result_t clay_measure_text(const std::string &text, Clay_TextElementConfig *config) {
    return app_api_clay_measure_text(this, text.c_str(), text.size(), config);
  }
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
                        get_on_leave_ptr<_class>(),                                       \
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

/**
 * Base class for binding apps
 *
 * @tparam Child The derived class
 * @tparam Adapter The adapter class that interfaces with the app
 * @tparam Ext The file extension for the loaded apps
 */
template<typename Child, typename Adapter, StringLiteral Ext>
class binding_app {
public:
  static constexpr auto file_ext = Ext.value;
  using adapter_t = Adapter;

private:
  std::vector<app_descriptor> descriptors;
  std::vector<std::string> app_names;

  static void adapter_teardown(void *userptr, const app_api_t controller_api) {
    if (userptr == nullptr)
      return;
    auto *adapter = static_cast<Adapter *>(userptr);
    if constexpr (HasOnTeardown<adapter_t>)
      adapter->teardown(controller_api);
    delete adapter;
  }

  static app_descriptor *
  load_app_wrapper(void *userptr, const app_api_t controller_api, const char *app_path, void **app_userptr) {
    return static_cast<Child *>(userptr)->load_app(controller_api, app_path, app_userptr);
  }

public:
  binding_app() = default;

  /**
   * Constructor that registers the app loader with the controller API
   *
   * @param controller_api The controller API object
   */
  explicit binding_app(app_api_t controller_api) {
    controller_api->register_app_loader(file_ext, &load_app_wrapper, this);
  }

protected:
  /**
   * Build an app descriptor with custom callbacks. You only need to use this if you need to override the default
   * callbacks provided by the adapter.
   *
   * @param app_name The name of the app
   * @param on_enter The on_enter callback
   * @param on_leave The on_leave callback
   * @param on_keypress The on_keypress callback
   * @return A pointer to the built app descriptor
   */
  app_descriptor *build_descriptor(const std::string &app_name,
                                   const app_on_enter_fn_t on_enter,
                                   const app_on_leave_fn_t on_leave,
                                   const app_on_keypress_fn_t on_keypress) {
    app_names.push_back(app_name);
    descriptors.push_back({
      .name = app_names.back().c_str(),
      .on_teardown = &adapter_teardown,
      .on_enter = on_enter,
      .on_leave = on_leave,
      .on_keypress = on_keypress,
    });
    return &descriptors.back();
  }

  /**
   * Build an app descriptor with default callbacks for the adapter specified by the template parameter
   *
   * @param app_name The name of the app
   * @return A pointer to the built app descriptor
   */
  app_descriptor *build_descriptor(const std::string &app_name) {
    return build_descriptor(app_name,
                            get_on_enter_ptr<Adapter>(),
                            get_on_leave_ptr<Adapter>(),
                            get_on_keypress_ptr<Adapter>());
  }
};
