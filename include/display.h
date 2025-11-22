#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <atomic>
#include <span>
#include <thread>
#include <vector>

#include "timer.h"

static constexpr size_t FRAMEBUFFER_COUNT = 2;

class Display {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  TTF_Font *font;

  void timer_thread_loop();
  std::atomic_bool running = true;
  mutable std::mutex thread_mutex;
  std::thread timer_thread;
  std::vector<timer> timers;

  uint8_t brightness = 255;
  mutable std::mutex fb_mutex;
  std::array<std::vector<uint32_t>, FRAMEBUFFER_COUNT> framebuffers;
  size_t current_framebuffer = 0;
  std::atomic_bool repaint_pending = false;

  SDL_Keycode button_down = SDLK_UNKNOWN;
  std::chrono::steady_clock::time_point button_down_time;

  void handle_keyevent(const SDL_Event &event);
  void repaint_if_pending() const;
  void on_quit();

public:
  Display();
  ~Display();

  void reset_display();
  void set_brightness(uint8_t value);
  void paint_bgr565(const std::span<uint16_t> &buf);
  void paint_rgb888(const std::span<uint32_t> &buf);

  uint32_t schedule(timer::callback_t &&callback, uint32_t interval, bool repeat);
  bool cancel(uint32_t timer_id);
  bool cancel_all();

  void dispatch_button(int button_id, bool use_timer = false);

  void loop();
  void run_forever();
};
