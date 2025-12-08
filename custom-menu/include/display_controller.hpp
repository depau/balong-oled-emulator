#pragma once
#include <functional>
#include <iostream>
#include <memory>

#include "clay.hpp"
#include "clay_fb_renderer.hpp"
#include "fonts/poppins_12.hpp"
#include "hooked_functions.h"

class display_controller {
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

  bool is_small_screen_mode = false;
  bool is_active = false;

public:
  display_controller();

  // ReSharper disable once CppMemberFunctionMayBeStatic
  [[nodiscard]] uint8_t width() const { return LCD_WIDTH; }
  [[nodiscard]] uint8_t height() const { return is_small_screen_mode ? LCD_HEIGHT / 2 : LCD_HEIGHT; }
  [[nodiscard]] bool active() const { return is_active; }
  [[nodiscard]] bool is_own_screen(const lcd_screen *screen) const { return screen == &secret_screen; }
  [[nodiscard]] bool is_small_screen() const { return is_small_screen_mode; }
  [[nodiscard]] const font_registry_t &get_font_registry() const { return font_registry; }

  void clay_render(const Clay_RenderCommandArray &cmds) {
    if (is_small_screen_mode) {
      ClayBW1Renderer renderer(secret_screen_buf, font_registry);
      renderer.clear(false);
      renderer.render(cmds);
    } else {
      ClayBGR565Renderer renderer(secret_screen_buf, font_registry);
      renderer.clear(Clay_Color{ 0, 0, 0, 255 });
      renderer.render(cmds);
    }
    lcd_refresh_screen(&secret_screen);
  }

  void set_active(const bool active) { is_active = active; }

  void switch_to_small_screen_mode() {
    if (is_small_screen_mode)
      return;

    is_small_screen_mode = true;
    secret_screen = { .sx = 0,
                      .height = height(),
                      .sy = 0,
                      .width = width(),
                      .buf_len = static_cast<uint16_t>(width() * height() / 8),
                      .buf = secret_screen_buf };

    Clay_SetLayoutDimensions(Clay_Dimensions{ 128, 64 });
  }

  void refresh_screen() const { lcd_refresh_screen(&secret_screen); }

  void on_keypress(int button);
};
