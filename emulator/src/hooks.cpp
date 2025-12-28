#include <cassert>
#include <iostream>
#include <memory>
#include <utility>

#include "debug.h"
#include "display.h"
#include "hooked_functions.h"
#include "hooks.h"

std::unique_ptr<Display> display = nullptr;
notify_handler_cb *hooked_notify_handler_async = nullptr;

void set_display(std::unique_ptr<Display> &&value) {
  display = std::move(value);
}

Display &get_display() {
  return *display;
}

static int notify_handler(const int subsystemid, const int action, int subaction) {
  assert(subsystemid == SUBSYSTEM_GPIO && "Unrecognised subsystem");
  display->dispatch_button(action);
  return 0;
}

void setup_hooks() {
  register_notify_handler(SUBSYSTEM_GPIO, nullptr, notify_handler);
}

void lcd_refresh_screen(const lcd_screen *screen) {
  if (display->is_short_screen_mode()) {
    display->paint_bw1bit(std::span(screen->buf, (screen->buf_len) / sizeof(uint16_t)));
  } else {
    display->paint_bgr565(std::span(screen->buf, (screen->buf_len) / sizeof(uint16_t)));
  }
}

int lcd_control_operate(const int mode) {
  switch (mode) {
  case LED_ON:
  case LED_ON - 100:
    display->set_brightness(255);
    break;
  case LED_DIM:
  case LED_DIM - 100:
    display->set_brightness(128);
    break;
  case LED_SLEEP:
  case LED_SLEEP - 100:
    display->set_brightness(10);
    break;
  default:
    std::cerr << "Unknown LCD control mode: " << mode << '\n';
  }
  return 0;
}

int register_notify_handler(int subsystemid, void *notify_handler_sync, notify_handler_cb *notify_handler_async) {
  hooked_notify_handler_async = notify_handler_async;
  return 0;
}

int call_notify_handler(const int subsystemid, const int action) {
  assert(subsystemid == SUBSYSTEM_GPIO && "Unrecognised subsystem");
  assert(hooked_notify_handler_async != nullptr);
  return hooked_notify_handler_async(subsystemid, action, 1);
}

uint32_t
osa_timer_create_ex(const uint32_t time, const uint32_t repeat, void (*callback)(void *userptr), void *userptr) {
  debugf("emulator: osa_timer_create_ex: time=%u, repeat=%u, callback=%p, userptr=%p\n",
         time,
         repeat,
         reinterpret_cast<void *>(callback),
         userptr);
  return display->schedule(callback, static_cast<int>(time), repeat != 0, userptr);
}

uint32_t osa_timer_delete_ex(const uint32_t timer_id) {
  return display->cancel(timer_id);
}

uint32_t osa_get_msgQ_id(const uint32_t queue_id) {
  return queue_id;
}

uint32_t osa_msgQex_send(uint32_t queue, uint32_t *msg, uint32_t len, uint32_t) {
  assert(len = 2 * sizeof(uint32_t));
  if (msg[0] == UI_MENU_EXIT) {
    std::cout << "Got UI_MENU_EXIT message" << std::endl;
    // Trigger callback to hijack
    call_notify_handler(SUBSYSTEM_GPIO, BUTTON_LONGPOWER);
  } else {
    std::cerr << "Got unknown message: " << msg[0] << '\n';
  }
  return 0;
}
