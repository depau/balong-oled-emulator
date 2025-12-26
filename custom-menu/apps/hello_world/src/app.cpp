#include <iostream>

#include "apps/app_api.hpp"
#include "symbols.h"
#include "symbols_demo.hpp"
#include "ui/actions/button.hpp"
#include "ui/screens/menu_screen.hpp"
#include "ui/ui_session.hpp"
#include "ui/ui_session_app.hpp"

class hello_world_app : public ui::ui_session_app<hello_world_app> {
  ui::screens::menu_screen::actions_vector_t menu_actions;

public:
  hello_world_app() = default;
  explicit hello_world_app(const app_api_t controller_api) : ui_session_app(controller_api) {}

  void setup(app_api_t controller_api) {
    menu_actions.emplace_back(std::make_unique<ui::actions::button>(GLYPH_ARROW_BACK " Back", [controller_api] {
      controller_api->goto_main_menu();
    }));
    menu_actions.emplace_back(std::make_unique<ui::actions::button>("Symbols demo", [this] {
      get_ui_session().push_screen(std::make_unique<symbols_demo>(get_ui_session()));
    }));

    get_ui_session().push_screen(std::make_unique<ui::screens::menu_screen>(menu_actions, "Hello World"));
  }
};

DECLARE_CPP_APP("Hello World", hello_world_app);
