#pragma once
#include <cstdint>

#define UNUSED(x) (void) (x)
#define NOINLINE __attribute__((noinline))

constexpr int LCD_WIDTH = 128;
constexpr int LCD_HEIGHT = 128;

constexpr int SUBSYSTEM_GPIO = 21002;

constexpr int BUTTON_POWER = 8;
constexpr int BUTTON_LONGPOWER = 22;
constexpr int BUTTON_LONGLONGPOWER = 4;
constexpr int BUTTON_MENU = 9;
constexpr int BUTTON_LONGMENU = 15;

constexpr int LED_ON = 100;
constexpr int LED_DIM = 101;
constexpr int LED_SLEEP = 102;

constexpr uint32_t UI_MENU_EXIT = 1006;

using notify_handler_cb = int(int subsystemid, int action, int subaction);

extern "C" {
struct lcd_screen {
  uint32_t sx;
  uint32_t height;
  uint32_t sy;
  uint32_t width;
  uint32_t buf_len;
  uint16_t *buf; // RGB565/BGR565 buffer
};

void NOINLINE lcd_refresh_screen(const lcd_screen *);
int NOINLINE lcd_control_operate(int lcd_mode);

int NOINLINE register_notify_handler(int subsystemid,
                                     void *notify_handler_sync,
                                     notify_handler_cb *notify_handler_async);
int NOINLINE call_notify_handler(int subsystemid, int action);

uint32_t NOINLINE osa_timer_create_ex(uint32_t time, uint32_t repeat, void (*callback)(void *userptr), void *userptr);
uint32_t NOINLINE osa_timer_delete_ex(uint32_t timer_id);
uint32_t NOINLINE osa_get_msgQ_id(uint32_t queue_id);
uint32_t NOINLINE osa_msgQex_send(uint32_t queue_id, uint32_t *msg, uint32_t size, uint32_t _);
}
