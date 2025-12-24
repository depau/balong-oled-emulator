#pragma once

#include "apps/app_api.hpp"
#include "ui/ui_session.hpp"

extern "C" app_descriptor_t *register_main_menu_app(app_api_t controller_api, void **userptr);

class main_menu_app {
  ui::ui_session session;
  ui::screens::menu_screen::actions_vector_t actions;
  ui::screens::menu_screen *menu_screen = nullptr;

public:
  main_menu_app() = default;
  explicit main_menu_app(const app_api_t controller_api);
  void load_app_actions(display_controller &controller);

  void on_enter(app_api_t);

  void on_keypress(app_api_t, const int button) const;
};
