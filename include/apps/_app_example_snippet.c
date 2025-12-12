#include <stdlib.h>

#include "apps/app_api.h"
#include "clay.h"

struct my_app_data {
  int some_value;
};

void *my_app_setup(app_api_t controller_api) {
  struct my_app_data *data = (struct my_app_data *) malloc(sizeof(struct my_app_data));
  data->some_value = 42;
  return data;
}

static void my_app_teardown(void *userptr, app_api_t controller_api) {
  free(userptr);
}

static void my_app_on_enter(void *userptr, app_api_t controller_api) {
  struct my_app_data *data = (struct my_app_data *) userptr;
  // Do something when the app gains focus, i.e.
  Clay_RenderCommandArray *cmds = /* foo */ NULL;
  app_api_clay_render(controller_api, cmds); // Example usage of controller_api
}

static void my_app_on_leave(void *userptr, app_api_t controller_api) {
  struct my_app_data *data = (struct my_app_data *) userptr;
  // Do something when the app loses focus
}

static void my_app_on_keypress(void *userptr, app_api_t controller_api, int button) {
  struct my_app_data *data = (struct my_app_data *) userptr;
  // Handle keypress event
}

DECLARE_APP("My App",
               my_app_setup,
               my_app_teardown,
               my_app_on_enter,
               my_app_on_leave,
               my_app_on_keypress);
