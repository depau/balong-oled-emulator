#include <stdlib.h>

#include "clay.h"
#include "plugins/plugin_api.h"

struct my_plugin_data {
  int some_value;
};

void *my_plugin_setup(plugin_api_t controller_api) {
  struct my_plugin_data *data = (struct my_plugin_data *) malloc(sizeof(struct my_plugin_data));
  data->some_value = 42;
  return data;
}

static void my_plugin_teardown(void *userptr, plugin_api_t controller_api) {
  free(userptr);
}

static void my_plugin_on_focus(void *userptr, plugin_api_t controller_api) {
  struct my_plugin_data *data = (struct my_plugin_data *) userptr;
  // Do something when the plugin gains focus, i.e.
  Clay_RenderCommandArray *cmds = /* foo */ NULL;
  plugin_api_clay_render(controller_api, cmds); // Example usage of controller_api
}

static void my_plugin_on_blur(void *userptr, plugin_api_t controller_api) {
  struct my_plugin_data *data = (struct my_plugin_data *) userptr;
  // Do something when the plugin loses focus
}

static void my_plugin_on_keypress(void *userptr, plugin_api_t controller_api, int button) {
  struct my_plugin_data *data = (struct my_plugin_data *) userptr;
  // Handle keypress event
}

DECLARE_PLUGIN("My Plugin",
               my_plugin_setup,
               my_plugin_teardown,
               my_plugin_on_focus,
               my_plugin_on_blur,
               my_plugin_on_keypress);
