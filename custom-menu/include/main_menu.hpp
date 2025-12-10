#pragma once

#include "plugins/plugin_api.hpp"

extern "C" plugin_descriptor_t *register_main_menu_plugin(plugin_api_t controller_api, void **userptr);

class main_menu_plugin {};