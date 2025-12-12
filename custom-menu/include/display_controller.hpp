#pragma once
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "clay.hpp"
#include "clay_fb_renderer.hpp"
#include "clay_utils.hpp"
#include "fonts/poppins_12.hpp"
#include "hooked_functions.h"
#include "plugins/plugin_api.hpp"

DECLARE_FN_TYPE(plugin_register_fn_t, plugin_descriptor_t *, plugin_api_t controller_api, void **userptr);

class display_controller : display_controller_api {
  friend class main_menu_plugin;

  using plugin_loader_desc_t = std::pair<plugin_loader_callback_fn_t, void *>;

  uint16_t secret_screen_buf[LCD_WIDTH * LCD_HEIGHT]{};
  lcd_screen secret_screen{ .sx = 1,
                            .height = 128,
                            .sy = 1,
                            .width = 128,
                            .buf_len = LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t),
                            .buf = secret_screen_buf };
  font_registry_t font_registry{ &fonts::Poppins_12 };

  std::size_t clay_arena_size = Clay_MinMemorySize();
  std::unique_ptr<void, decltype(&std::free)> clay_arena_memory{ std::malloc(clay_arena_size), std::free };
  Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(clay_arena_size, clay_arena_memory.get());

  // ReSharper disable once CppPassValueParameterByConstReference
  Clay_ErrorHandler errHandler = { [](const Clay_ErrorData error_data) {
                                    fprintf(stderr, "clay error: ");
                                    fwrite(error_data.errorText.chars, 1, error_data.errorText.length, stderr);
                                    fputc('\n', stderr);
                                  },
                                   nullptr };

  std::vector<std::string> plugin_lookup_paths = get_plugin_lookup_paths();
  std::vector<std::pair<plugin_descriptor, void *>> plugins{};
  std::map<std::string, plugin_loader_desc_t> plugin_loaders{};
  std::optional<size_t> active_plugin_index = std::nullopt;

  std::optional<std::string> plugin_error_message = std::nullopt;

  bool is_small_screen_mode = false;
  bool is_active = false;

  static std::vector<std::string> get_plugin_lookup_paths();

  void load_plugins();

  void set_active_plugin(std::optional<size_t> plugin_index);

public:
  display_controller();
  ~display_controller();

  // Forbid copying
  display_controller(const display_controller &) = delete;
  display_controller &operator=(const display_controller &) = delete;

  // Allow moving
  display_controller(display_controller &&) = default;
  display_controller &operator=(display_controller &&) = default;

  // ReSharper disable once CppMemberFunctionMayBeStatic
  // NOLINTNEXTLINE(*-convert-member-functions-to-static)
  [[nodiscard]] uint8_t width() const { return LCD_WIDTH; }
  [[nodiscard]] uint8_t height() const { return is_small_screen_mode ? LCD_HEIGHT / 2 : LCD_HEIGHT; }
  [[nodiscard]] bool active() const { return is_active; }
  [[nodiscard]] bool is_own_screen(const lcd_screen *screen) const { return screen == &secret_screen; }
  [[nodiscard]] bool is_small_screen() const { return is_small_screen_mode; }
  [[nodiscard]] const font_registry_t &get_font_registry() const { return font_registry; }

  void clay_render(const Clay_RenderCommandArray &cmds);

  void draw_frame(const std::span<const uint16_t> &buf) {
    std::ranges::copy(buf, secret_screen_buf);
    lcd_refresh_screen(&secret_screen);
  }

  void draw_frame(const std::span<uint16_t> &buf) {
    return draw_frame(static_cast<const std::span<const uint16_t>>(buf));
  }

  [[nodiscard]] std::optional<uint16_t> get_font(const std::string &fontName, int fontSize) const;

  void set_active(const bool active);

  void switch_to_small_screen_mode();

  void refresh_screen() const { lcd_refresh_screen(&secret_screen); }

  void on_keypress(int button);

  void register_plugin_loader(const std::string &file_extension,
                              plugin_loader_callback_fn_t loader_fn,
                              void *userptr = nullptr) {
    plugin_loaders[file_extension] = std::make_pair(loader_fn, userptr);
  }

  void goto_main_menu();

  void fatal_error(const char *message, bool unload_plugin);
};
