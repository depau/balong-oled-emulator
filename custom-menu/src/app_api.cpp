#include "apps/app_api.hpp"
#include "display_controller.hpp"

static display_controller &get_display_controller(const app_api_t controller_api) {
  return *reinterpret_cast<display_controller *>(controller_api);
}

static const display_controller &get_display_controller(const c_app_api_t controller_api) {
  return *reinterpret_cast<const display_controller *>(controller_api);
}

display_mode_t app_api_get_display_mode(const c_app_api_t controller_api) {
  return get_display_controller(controller_api).is_small_screen() ? DISPLAY_MODE_BW1 : DISPLAY_MODE_BGR565;
}

// ReSharper disable once CppDFAConstantFunctionResult
size_t app_api_get_screen_width(const c_app_api_t controller_api) {
  return get_display_controller(controller_api).width();
}

size_t app_api_get_screen_height(const c_app_api_t controller_api) {
  return get_display_controller(controller_api).height();
}

uint16_t app_api_get_font(const c_app_api_t controller_api, const char *font_name, const int font_size) {
  const auto result = get_display_controller(controller_api).get_font(font_name, font_size);
  if (result.has_value())
    return result.value();
  return FONT_NOT_FOUND;
}

void app_api_clay_render(const app_api_t controller_api, const Clay_RenderCommandArray *cmds) {
  get_display_controller(controller_api).clay_render(*cmds);
}

void app_api_draw_frame(const app_api_t controller_api, const uint16_t *buf, const size_t buf_size) {
  get_display_controller(controller_api).draw_frame(std::span(buf, buf_size));
}

void app_api_goto_main_menu(app_api_t controller_api) {
  get_display_controller(controller_api).goto_main_menu();
}

void app_api_fatal_error(app_api_t controller_api, const char *message, bool unload_app) {
  get_display_controller(controller_api).fatal_error(message, unload_app);
}

void app_api_register_app_loader(const app_api_t controller_api,
                                 const char *file_extension,
                                 const app_loader_callback_fn_t loader_fn,
                                 void *userptr) {
  get_display_controller(controller_api).register_app_loader(file_extension, loader_fn, userptr);
}
