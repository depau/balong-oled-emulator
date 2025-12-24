#pragma once
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "apps/app_api.hpp"
#include "clay_fb_renderer.hpp"
#include "fonts/poppins_12.hpp"
#include "hooked_functions.h"
#include "stdfn_timer_helper.hpp"

DECLARE_FN_TYPE(app_register_fn_t, app_descriptor_t *, app_api_t controller_api, void **userptr);

class display_controller : display_controller_api {
  friend class main_menu_app;
  friend class stdfn_timer_helper<display_controller>;

  using app_loader_desc_t = std::pair<app_loader_callback_fn_t, void *>;

  uint32_t (*timer_create_ex)(uint32_t, uint32_t, void (*)(void *), void *);
  uint32_t (*timer_delete_ex)(uint32_t);
  uint32_t (*get_msgQ_id)(uint32_t);
  uint32_t (*msgQex_send)(uint32_t, uint32_t *, uint32_t, uint32_t);

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

  std::vector<std::string> app_lookup_paths = get_app_lookup_paths();
  std::vector<std::pair<app_descriptor, void *>> apps{};
  std::map<std::string, app_loader_desc_t> app_loaders{};
  std::optional<size_t> active_app_index = std::nullopt;

  std::optional<std::string> app_error_message = std::nullopt;

  std::mutex stdfn_timer_mutex;
  std::map<uint32_t, stdfn_timer_helper<display_controller>> scheduled_stdfn_timers{};
  std::map<uint32_t, uint32_t> scheduled_stdfn_timer_ids{};
  uint32_t next_stdfn_timer_id = 1;

  bool is_small_screen_mode = false;
  bool is_active = false;

  // Private methods
  static std::vector<std::string> get_app_lookup_paths();

  void load_apps();

  void set_active_app(std::optional<size_t> app_index);

  void gc_stdfn_timer(uint32_t stdfn_timer_id);

  void send_msg(const uint32_t msg_type) const;

public:
  display_controller();
  ~display_controller();

  // Forbid copying
  display_controller &operator=(const display_controller &) = delete;
  display_controller(const display_controller &) = delete;

  // Forbid moving
  display_controller &operator=(display_controller &&) = delete;
  display_controller(display_controller &&) = delete;

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
    std::copy(buf.begin(), buf.end(), secret_screen_buf);
    lcd_refresh_screen(&secret_screen);
  }

  void draw_frame(const std::span<uint16_t> &buf) {
    return draw_frame(static_cast<const std::span<const uint16_t>>(buf));
  }

  [[nodiscard]] std::optional<uint16_t> get_font(const std::string &fontName, int fontSize) const;

  void set_active(bool active);

  void switch_to_small_screen_mode();

  void refresh_screen() const { lcd_refresh_screen(&secret_screen); }

  void on_keypress(int button);

  void
  register_app_loader(const std::string &file_extension, app_loader_callback_fn_t loader_fn, void *userptr = nullptr) {
    app_loaders[file_extension] = std::make_pair(loader_fn, userptr);
  }

  void goto_main_menu();

  void fatal_error(const char *message, bool unload_app);

  uint32_t schedule_timer(std::function<void()> &&callback, uint32_t interval_ms, bool repeat = false);

  uint32_t
  schedule_timer(void (*callback)(void *userptr), uint32_t interval_ms, bool repeat = false, void *userptr = nullptr);

  uint32_t cancel_timer(uint32_t timer_id);

  const std::vector<std::pair<app_descriptor, void *>> &get_apps() { return apps; }
};
