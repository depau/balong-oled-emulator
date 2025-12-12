#pragma once

#include "apps/app_api.hpp"

extern "C" app_descriptor_t *register_main_menu_app(app_api_t controller_api, void **userptr);

class main_menu_app {
public:
  void on_enter(app_api_t controller_api);
  void on_leave(app_api_t controller_api);
  void on_keypress(app_api_t controller_api, int button);
};