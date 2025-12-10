#pragma once

#include <optional>
#include <span>
#include <utility>

#include "plugin_api.h"

struct display_controller_api {
  /**
   * Get the current display mode
   *
   * @return The current display mode
   */
  [[nodiscard]] display_mode_t get_display_mode() const { return plugin_api_get_display_mode(this); }

  /**
   * Get the screen width in pixels
   *
   * @return The screen width in pixels
   */
  [[nodiscard]] size_t get_screen_width() const { return plugin_api_get_screen_width(this); }

  /**
   * Get the screen height in pixels
   *
   * @return The screen height in pixels
   */
  [[nodiscard]] size_t get_screen_height() const { return plugin_api_get_screen_height(this); }

  /**
   * Get a font ID by name and size
   *
   * @param font_name The name of the font
   * @param font_size The size of the font
   * @return The font ID, or FONT_NOT_FOUND if not found
   */
  [[nodiscard]] std::optional<uint16_t> get_font(const char *font_name, const int font_size) const {
    const uint16_t result = plugin_api_get_font(this, font_name, font_size);
    if (result == FONT_NOT_FOUND)
      return std::nullopt;
    return result;
  };

  /**
   * Render a frame using Clay rendering commands
   *
   * @param cmds The Clay rendering commands
   */
  void clay_render(const Clay_RenderCommandArray &cmds) { plugin_api_clay_render(this, &cmds); }

  /**
   * Draw a raw frame buffer to the screen
   *
   * @param buf The frame buffer data
   */
  void draw_frame(const std::span<uint16_t> &buf) { plugin_api_draw_frame(this, buf.data(), buf.size()); }

  /**
   * Draw a raw frame buffer to the screen
   *
   * @param buf The frame buffer data
   */
  void draw_frame(const std::span<const uint16_t> &buf) { plugin_api_draw_frame(this, buf.data(), buf.size()); }

  /**
   * Return to the main menu, leaving the current plug-in. `on_blur` will be called before returning.
   */
  void goto_main_menu() { plugin_api_goto_main_menu(this); }

  /**
   * Report a fatal error to the controller.
   *
   * Renders an error message to the UI and exits the plug-in, calling `on_blur`. If `unload_plugin` is true, the
   * plug-in will be unloaded after reporting the error, calling its `on_teardown` function, and removing it from the
   * list of loaded plug-ins.
   *
   * @param message The error message to display
   * @param unload_plugin Whether to unload the plug-in after reporting the error
   */
  void fatal_error(const char *message, const bool unload_plugin) {
    plugin_api_fatal_error(this, message, unload_plugin);
  }

  /**
   * Register a plug-in loader for a specific file extension
   *
   * @param file_extension The file extension to register (including the dot, e.g. ".so")
   * @param loader_fn The loader function to call when loading a plug-in with the specified extension
   * @param userptr A user pointer to pass to the loader function
   */
  void register_plugin_loader(const char *file_extension, const plugin_loader_callback_fn_t loader_fn, void *userptr) {
    plugin_api_register_plugin_loader(this, file_extension, loader_fn, userptr);
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
concept HasOnFocus = requires(T &t, plugin_api_t controller_api) {
  { t.on_focus(controller_api) } -> std::same_as<void>;
};

template<typename T>
concept HasOnBlur = requires(T &t, plugin_api_t controller_api) {
  { t.on_blur(controller_api) } -> std::same_as<void>;
};

template<typename T>
concept HasOnKeypress = requires(T &t, plugin_api_t controller_api, int button) {
  { t.on_keypress(controller_api, button) } -> std::same_as<void>;
};

template<typename T>
concept HasOnTeardown = requires(T &t, plugin_api_t controller_api) {
  { t.on_teardown(controller_api) } -> std::same_as<void>;
};

template<typename T>
void on_focus_trampoline(void *userptr, plugin_api_t controller_api) {
  if constexpr (HasOnFocus<T>) {
    static_cast<T *>(userptr)->on_focus(controller_api);
  }
}

template<typename T>
void on_blur_trampoline(void *userptr, plugin_api_t controller_api) {
  if constexpr (HasOnBlur<T>) {
    static_cast<T *>(userptr)->on_blur(controller_api);
  }
}

template<typename T>
void on_keypress_trampoline(void *userptr, plugin_api_t controller_api, int button) {
  if constexpr (HasOnKeypress<T>) {
    static_cast<T *>(userptr)->on_keypress(controller_api, button);
  }
}

template<typename T>
void on_teardown_trampoline(void *userptr, plugin_api_t controller_api) {
  if constexpr (HasOnTeardown<T>) {
    static_cast<T *>(userptr)->on_teardown(controller_api);
  }
}

template<typename T>
consteval plugin_on_focus_fn_t get_on_focus_ptr() {
  if constexpr (HasOnFocus<T>) {
    return &on_focus_trampoline<T>;
  }
  return nullptr;
}

template<typename T>
consteval plugin_on_blur_fn_t get_on_blur_ptr() {
  if constexpr (HasOnBlur<T>) {
    return &on_blur_trampoline<T>;
  }
  return nullptr;
}

template<typename T>
consteval plugin_on_keypress_fn_t get_on_keypress_ptr() {
  if constexpr (HasOnKeypress<T>) {
    return &on_keypress_trampoline<T>;
  }
  return nullptr;
}

// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _DECLARE_CPP_PLUGIN_INTERNAL(_register_plugin_fn_name, _descriptor_name, _name, _class) \
  void *_##_class##__setup(plugin_api_t controller_api) {                                       \
    return construct_maybe_with_arg<_class>(controller_api);                                    \
  }                                                                                             \
  void _##_class##__teardown(void *userptr, plugin_api_t controller_api) {                      \
    on_teardown_trampoline<_class>(userptr, controller_api);                                    \
    delete reinterpret_cast<_class *>(userptr);                                                 \
  }                                                                                             \
  _DECLARE_PLUGIN_INTERNAL(_register_plugin_fn_name,                                            \
                           _descriptor_name,                                                    \
                           _name,                                                               \
                           _##_class##__setup,                                                  \
                           _##_class##__teardown,                                               \
                           get_on_focus_ptr<_class>(),                                          \
                           get_on_blur_ptr<_class>(),                                           \
                           get_on_keypress_ptr<_class>())

/**
 * Declare a C++ plug-in.
 *
 * @snippet _plugin_example_snippet.cpp Demonstration of DECLARE_CPP_PLUGIN usage
 *
 * @param _name The name of the plug-in
 * @param _class The plug-in class name
 */
#define DECLARE_CPP_PLUGIN(_name, _class) \
  _DECLARE_CPP_PLUGIN_INTERNAL(register_plugin, plugin_descriptor, _name, _class)
