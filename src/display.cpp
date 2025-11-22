#include <chrono>
#include <iostream>
#include <mutex>
#include <span>
#include <vector>

#include "display.h"
#include "hooks.h"
#include "sdl_utils.h"
#include "thread_name.h"

using namespace std::chrono;
using namespace std::chrono_literals;

constexpr int FPS = 60;
constexpr int FRAME_TIME_MS = 1000 / FPS;

constexpr auto HOLD_TIME = 500ms;
constexpr auto LONG_HOLD_TIME = 1s;

constexpr SDL_Keycode KEY_POWER = SDLK_RETURN;
constexpr SDL_Keycode KEY_MENU = SDLK_SPACE;

Display::Display() : timer_thread([&] { timer_thread_loop(); }) {
  window = SDL_CreateWindow("Balong OLED Emulator",
                            SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED,
                            LCD_WIDTH * 2,
                            LCD_HEIGHT * 2,
                            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

  if (!window) {
    std::cerr << "Could not create window: " << SDL_GetError() << '\n';
    abort();
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer) {
    std::cerr << "Could not create renderer: " << SDL_GetError() << '\n';
    abort();
  }

  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, LCD_WIDTH, LCD_HEIGHT);
  if (!texture) {
    std::cerr << "Could not create texture: " << SDL_GetError() << '\n';
    abort();
  }

  const std::string font_path = find_sans_serif_font_path();
  font = TTF_OpenFont(font_path.c_str(), 18);
  if (!font) {
    std::cerr << "Could not load font: " << TTF_GetError() << '\n';
    abort();
  }

  reset_display();
}

Display::~Display() {
  on_quit();

  /// Leaking memory since I don't want to fix the SIGSEGV
  // TTF_CloseFont(font);
  // SDL_DestroyTexture(texture);
  // SDL_DestroyRenderer(renderer);
  // SDL_DestroyWindow(window);
}

void Display::reset_display() {
  std::vector<uint32_t> rgb888_buffer(LCD_WIDTH * LCD_HEIGHT);

  fill_gradient(rgb888_buffer, LCD_WIDTH, LCD_HEIGHT);
  draw_text(rgb888_buffer, LCD_WIDTH, LCD_HEIGHT, is_short_screen_mode() ? "128x64 OLED" : "128x128 LCD", font);

  paint_rgb888(rgb888_buffer);
}

void Display::paint_bw1bit(const std::span<uint16_t> &buf) {
  std::vector<uint32_t> rgb888_short_buffer(LCD_WIDTH * lcd_height);
  convert_bw1bit_to_rgb888(buf, rgb888_short_buffer);

  // Center the drawn image into the larger buffer
  std::vector<uint32_t> gradient(LCD_WIDTH * LCD_HEIGHT);
  fill_gradient(gradient, LCD_WIDTH, lcd_height);

  const ssize_t offset = LCD_HEIGHT / 2 - lcd_height / 2;

  std::vector<uint32_t> rgb888_buffer;
  rgb888_buffer.reserve(LCD_WIDTH * LCD_HEIGHT);

  std::copy_n(gradient.begin(), offset * LCD_WIDTH, std::back_inserter(rgb888_buffer));
  std::ranges::copy(rgb888_short_buffer, std::back_inserter(rgb888_buffer));
  std::copy_n(gradient.begin(),
              LCD_HEIGHT * LCD_HEIGHT - (offset + lcd_height) * LCD_WIDTH,
              std::back_inserter(rgb888_buffer));

  paint_rgb888(rgb888_buffer);
}

void Display::paint_bgr565(const std::span<uint16_t> &buf) {
  std::vector<uint32_t> rgb888_buffer(LCD_WIDTH * LCD_HEIGHT);
  convert_bgr565_to_rgb888(buf, rgb888_buffer);
  paint_rgb888(rgb888_buffer);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Display::paint_rgb888(const std::span<uint32_t> &buf) {
  // ReSharper disable once CppTooWideScope
  std::vector copy(buf.begin(), buf.end());
  {
    std::scoped_lock lock(fb_mutex);
    const size_t new_fb_num = (current_framebuffer + 1) % FRAMEBUFFER_COUNT;
    framebuffers[new_fb_num] = std::move(copy);
    current_framebuffer = new_fb_num;
    repaint_pending = true;
  }
}

void Display::set_short_screen_mode(const bool enabled) {
  if (enabled)
    lcd_height = LCD_HEIGHT / 2;
  else
    lcd_height = LCD_HEIGHT;

  // Call lcd_refresh_screen() (hooked) to notify the hijack lib
  size_t fb_size = LCD_WIDTH * lcd_height;
  if (enabled)
    fb_size /= 8;
  else
    fb_size *= sizeof(uint16_t);

  std::vector<uint16_t> fb(fb_size / sizeof(uint16_t));

  const lcd_screen notification_screen{
    .sx = 0,
    .height = static_cast<uint32_t>(lcd_height),
    .sy = 0,
    .width = LCD_WIDTH,
    .buf_len = static_cast<uint32_t>(fb_size),
    .buf = fb.data(),
  };

  lcd_refresh_screen(&notification_screen);

  // Schedule reset to clear out the black screen
  schedule([&] { reset_display(); }, FRAME_TIME_MS, false);
}

void Display::set_brightness(const uint8_t value) {
  if (brightness != value) {
    brightness = value;
    repaint_pending = true;
  }
}

uint32_t Display::schedule(timer::callback_t &&callback, const uint32_t interval, const bool repeat) {
  std::scoped_lock lock(thread_mutex);
  timers.emplace_back(std::move(callback), interval, repeat);
  std::push_heap(timers.begin(), timers.end(), timer::compare_deadlines_reverse);
  return timers.back().get_id();
}

bool Display::cancel(const uint32_t timer_id) {
  std::scoped_lock lock(thread_mutex);
  for (auto it = timers.begin(); it != timers.end(); ++it) {
    if (it->get_id() == timer_id) {
      timers.erase(it);
      std::make_heap(timers.begin(), timers.end(), timer::compare_deadlines_reverse);
      return true;
    }
  }
  return false;
}

bool Display::cancel_all() {
  std::scoped_lock lock(thread_mutex);
  timers.clear();
  return true;
}

void Display::dispatch_button(const int button_id, bool use_timer) {
  std::vector<uint32_t> rgb888_buf(LCD_WIDTH * LCD_HEIGHT);

  std::string text;
  switch (button_id) {
  case BUTTON_POWER:
    text = "POWER";
    break;
  case BUTTON_LONGPOWER:
    text = "POWER HOLD";
    break;
  case BUTTON_LONGLONGPOWER:
    text = "POWER OFF";
    break;
  case BUTTON_MENU:
    text = "MENU";
    break;
  case BUTTON_LONGMENU:
    text = "MENU HOLD";
    break;
  default:
    text = "???";
  }

  set_brightness(255);
  fill_gradient(rgb888_buf, LCD_WIDTH, LCD_HEIGHT);
  draw_text(rgb888_buf, LCD_WIDTH, LCD_HEIGHT, text, font);

  paint_rgb888(rgb888_buf);

  if (use_timer)
    schedule([&] { reset_display(); }, 500, false);
}

void Display::timer_thread_loop() {
  set_thread_name("oled_timer");
  while (running) {
    std::unique_lock lock(thread_mutex);

    while (!timers.empty() && timers.front().is_expired()) {
      auto &t = timers.front();

      lock.unlock();
      t.run();
      lock.lock();

      if (t.should_repeat()) {
        t.reset();
        std::make_heap(timers.begin(), timers.end(), timer::compare_deadlines_reverse);
      } else {
        std::pop_heap(timers.begin(), timers.end(), timer::compare_deadlines_reverse);
        timers.pop_back();
      }
    }

    lock.unlock();

    if (!running)
      return;

    std::this_thread::sleep_for(FRAME_TIME_MS / 2 * 1ms);
  }
}

void Display::handle_keyevent(const SDL_Event &event) {
  const auto keycode = event.key.keysym.sym;

  if (keycode != KEY_POWER && keycode != KEY_MENU)
    return;

  if (event.type == SDL_KEYDOWN) {
    if (button_down == SDLK_UNKNOWN) {
      std::cout << "Key down: " << SDL_GetKeyName(keycode) << std::endl;
      button_down = event.key.keysym.sym;
      button_down_time = steady_clock::now();
    }

  } else if (event.type == SDL_KEYUP) {
    if (keycode == button_down) {
      std::cout << "Key up: " << SDL_GetKeyName(keycode) << std::endl;
      const auto hold_time = duration_cast<milliseconds>(steady_clock::now() - button_down_time);

      if (hold_time >= LONG_HOLD_TIME && button_down == KEY_POWER) {
        call_notify_handler(SUBSYSTEM_GPIO, BUTTON_LONGLONGPOWER);

      } else if (hold_time >= HOLD_TIME) {
        if (button_down == KEY_POWER) {
          call_notify_handler(SUBSYSTEM_GPIO, BUTTON_LONGPOWER);
        } else {
          call_notify_handler(SUBSYSTEM_GPIO, BUTTON_LONGMENU);
        }

      } else {
        if (button_down == KEY_POWER) {
          call_notify_handler(SUBSYSTEM_GPIO, BUTTON_POWER);
        } else {
          call_notify_handler(SUBSYSTEM_GPIO, BUTTON_MENU);
        }
      }

      button_down = SDLK_UNKNOWN;
    }
  }
}

void Display::repaint_if_pending() const {
  if (!repaint_pending)
    return;

  std::vector<uint32_t> fb;
  {
    std::scoped_lock lock(fb_mutex);
    fb = framebuffers[current_framebuffer];
  }
  if (brightness < 255) {
    dim_buffer(fb, brightness);
  }

  SDL_UpdateTexture(texture, nullptr, fb.data(), LCD_WIDTH * static_cast<int>(sizeof(uint32_t)));
}

void Display::on_quit() {
  running = false;
  if (timer_thread.joinable())
    timer_thread.join();
}

void Display::loop() {
  SDL_Event event;
  while (SDL_WaitEventTimeout(&event, FRAME_TIME_MS)) {
    if (event.type == SDL_QUIT) {
      on_quit();
    } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
      handle_keyevent(event);
    }
  }
  repaint_if_pending();

  int window_width = 0;
  int window_height = 0;
  SDL_GetRendererOutputSize(renderer, &window_width, &window_height);

  constexpr float aspect_ratio = static_cast<float>(LCD_WIDTH) / static_cast<float>(LCD_HEIGHT);
  const float window_aspect_ratio = static_cast<float>(window_width) / static_cast<float>(window_height);

  SDL_Rect dest_rect{};
  if (window_aspect_ratio > aspect_ratio) {
    dest_rect.h = window_height;
    dest_rect.w = static_cast<int>(static_cast<float>(window_height) * aspect_ratio);
    dest_rect.y = 0;
    dest_rect.x = (window_width - dest_rect.w) / 2;
  } else {
    dest_rect.w = window_width;
    dest_rect.h = static_cast<int>(static_cast<float>(window_width) / aspect_ratio);
    dest_rect.x = 0;
    dest_rect.y = (window_height - dest_rect.h) / 2;
  }

  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, nullptr, &dest_rect);
  SDL_RenderPresent(renderer);
}

void Display::run_forever() {
  set_thread_name("oled_main");
  while (running) {
    loop();
  }
}
