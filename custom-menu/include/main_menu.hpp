#pragma once

#include "plugins/plugin_api.hpp"

extern "C" plugin_descriptor_t *register_main_menu_plugin(plugin_api_t controller_api, void **userptr);

class main_menu_plugin {
public:
  void on_focus(plugin_api_t controller_api);
  void on_blur(plugin_api_t controller_api);
  void on_keypress(plugin_api_t controller_api, int button);
};