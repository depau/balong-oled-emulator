#include <cstdint>
#include <cstdlib>
#include <dlfcn.h>
#include <iostream>
#include <sys/capability.h>
#include <sys/types.h>
#include <unistd.h>

#include "display_controller.hpp"
#include "hooked_functions.h"

display_controller screen_controller{};

int (*notify_handler_async_real)(int subsystemid, int action, int subaction) = nullptr;
void (*lcd_refresh_screen_real)(const lcd_screen *screen) = nullptr;
int (*lcd_control_operate_real)(int lcd_mode) = nullptr;

uint32_t (*get_msgQ_id)(uint32_t) = nullptr;
uint32_t (*msgQex_send)(uint32_t, uint32_t *, uint32_t, uint32_t) = nullptr;

extern "C" int lcd_control_operate(const int lcd_mode) {
  // we use other values in secret mode to have full control on lcd
  if (screen_controller.active()) {
    if (lcd_mode < 100)
      return 0;
    return lcd_control_operate_real(lcd_mode - 100);
  }
  if (lcd_mode >= 100)
    return 0;
  return lcd_control_operate_real(lcd_mode);
}

extern "C" NOINLINE void lcd_refresh_screen(const lcd_screen *screen) {
  // Ignore our screen's refreshes when the original screen is active
  if (!screen_controller.active() && screen_controller.is_own_screen(screen)) {
    return;
  }
  // Ignore original screen refreshes when our screen is active
  if (screen_controller.active() && !screen_controller.is_own_screen(screen)) {
    return;
  }
  // Detect and report small display devices
  if (!screen_controller.active() && screen && screen->buf_len == 1024) {
    screen_controller.switch_to_small_screen_mode();
  }
  // If everything's good, actually perform the refresh
  lcd_refresh_screen_real(screen);
}

// ReSharper disable once CppDFAConstantParameter
static void send_msg(const uint32_t msg_type) {
  constexpr int DEFAULT_QUEUE_ID = 1001;

  const uint32_t msg_queue = get_msgQ_id(DEFAULT_QUEUE_ID);
  if (!msg_queue) {
    fprintf(stderr, "Failed to get message queue to close the menu\n");
    return;
  }
  uint32_t msg[2] = { msg_type, 0 };
  msgQex_send(msg_queue, msg, 2 * sizeof(uint32_t), 0);
}

/*
 * The main hijacked handler function.
 */
int notify_handler_async(int subsystemid, int action, int subaction) {
  std::cerr << "notify_handler_async: " << subsystemid << ", " << action << ", " << subaction << std::endl;

  if (subsystemid == SUBSYSTEM_GPIO) {
    if (action == BUTTON_LONGMENU) {
      screen_controller.set_active(!screen_controller.active());
      send_msg(UI_MENU_EXIT);
      // force restarting the LED brightness timer if already fired
      if (!screen_controller.is_small_screen()) {
        notify_handler_async_real(SUBSYSTEM_GPIO, BUTTON_POWER, 0);
      } else {
        notify_handler_async_real(SUBSYSTEM_GPIO, BUTTON_MENU, 0);
      }

      return 0;
    }

    if (screen_controller.active() && (action == BUTTON_MENU || action == BUTTON_POWER)) {
      screen_controller.on_keypress(action);
      return 0;
    }
  }

  return notify_handler_async_real(subsystemid, action, subaction);
}

extern "C" NOINLINE int
register_notify_handler(int subsystemid, void *notify_handler_sync, notify_handler_cb *notify_handler_async_orig) {
  std::cerr << "register_notify_handler: " << subsystemid << " - hooked" << std::endl;
  unsetenv("LD_PRELOAD");

  static int (*register_notify_handler_real)(int, void *, void *) = nullptr;
  register_notify_handler_real = reinterpret_cast<int (*)(int, void *, void *)>(
    dlsym(RTLD_NEXT, "register_notify_handler"));
  lcd_refresh_screen_real = reinterpret_cast<void (*)(const lcd_screen *screen)>(
    dlsym(RTLD_NEXT, "lcd_refresh_screen"));
  lcd_control_operate_real = reinterpret_cast<int (*)(int lcd_mode)>(dlsym(RTLD_NEXT, "lcd_control_operate"));

  get_msgQ_id = reinterpret_cast<uint32_t (*)(uint32_t)>(dlsym(RTLD_DEFAULT, "osa_get_msgQ_id"));
  msgQex_send = reinterpret_cast<uint32_t (*)(uint32_t, uint32_t *, uint32_t, uint32_t)>(
    dlsym(RTLD_DEFAULT, "osa_msgQex_send"));

  if (!register_notify_handler_real || !lcd_refresh_screen_real || !lcd_control_operate_real || !get_msgQ_id ||
      !msgQex_send) {
    std::cerr << "The program is not compatible with this device" << std::endl;
    return 1;
  }

  notify_handler_async_real = notify_handler_async_orig;
  return register_notify_handler_real(subsystemid, notify_handler_sync, reinterpret_cast<void *>(notify_handler_async));
}

extern "C" int setuid(const uid_t __uid) {
  // put uid to saved to be able to restore it when needed
  return setresuid(__uid, __uid, 0);
}

extern "C" int setgid(const gid_t __gid) {
  return setresgid(__gid, __gid, 0);
}

extern "C" int prctl(const int option,
                     const unsigned long arg2,
                     const unsigned long arg3,
                     const unsigned long arg4,
                     const unsigned long arg5) {
  // not allowing to drop capabilities
  UNUSED(option);
  UNUSED(arg2);
  UNUSED(arg3);
  UNUSED(arg4);
  UNUSED(arg5);
  return -1;
}

extern "C" int capset(const cap_user_header_t header, const cap_user_data_t data) {
  data[0].effective |= (1 << CAP_NET_RAW) | (1 << CAP_NET_ADMIN);
  data[0].permitted |= (1 << CAP_NET_RAW) | (1 << CAP_NET_ADMIN);
  data[0].inheritable |= (1 << CAP_NET_RAW) | (1 << CAP_NET_ADMIN);
  auto *capset_real = reinterpret_cast<int (*)(cap_user_header_t, cap_user_data_t)>(dlsym(RTLD_NEXT, "capset"));
  if (!capset_real) {
    return -1;
  }

  return capset_real(header, data);
}

// No actual main(): this is a shared library loaded via LD_PRELOAD, the entry point is register_notify_handler().
