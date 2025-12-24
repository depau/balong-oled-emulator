#include <iostream>
#include <ranges>

#include "apps/app_api.hpp"
#include "display_controller.hpp"
#include "main_menu.hpp"
#include "symbols.h"
#include "ui/actions/button.hpp"

_DECLARE_CPP_APP_INTERNAL(register_main_menu_app, main_menu_app_descriptor, "Main Menu", main_menu_app);

main_menu_app::main_menu_app(const app_api_t controller_api) : session(*controller_api) {
  menu_screen = &session.push_screen(std::make_unique<ui::screens::menu_screen>(actions));
}

void main_menu_app::load_app_actions(display_controller &controller) {
  actions.clear();
  actions.emplace_back(std::make_unique<ui::actions::button>(GLYPH_ARROW_BACK " Back", [&controller, this] {
    controller.set_active(false);
    menu_screen->set_active_entry(0);
  }));

  int index = 0;
  for (const auto &descriptor : controller.get_apps() | std::views::keys) {
    if (index == 0) {
      ++index;
      continue; // skip main menu itself
    }
    actions.emplace_back(std::make_unique<ui::actions::button>(descriptor.name, [&controller, index] {
      controller.set_active_app(index);
    }));
    ++index;
  }
  debugf("registered %zu main menu actions\n", actions.size());
}

void main_menu_app::on_enter(const app_api_t controller_api) {
  assert(controller_api != nullptr);
  auto &controller = static_cast<display_controller &>(*controller_api);

  load_app_actions(controller);
  session.render();
}

void main_menu_app::on_keypress(app_api_t, const int button) const {
  session.handle_keypress(button);
}
