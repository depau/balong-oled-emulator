#pragma once
#include "plugins/plugin_api.hpp"

plugin_descriptor *
load_plugin_shared_object(void *userptr, plugin_api_t controller_api, const char *plugin_path, void **plugin_userptr);
