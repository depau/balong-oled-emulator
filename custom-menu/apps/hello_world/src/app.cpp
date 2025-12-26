#include <iostream>

#include "apps/app_api.hpp"
#include "symbols.h"
#include "symbols_demo.hpp"
#include "ui/actions/button.hpp"
#include "ui/actions/label.hpp"
#include "ui/actions/page_break.hpp"
#include "ui/actions/radio.hpp"
#include "ui/actions/toggle.hpp"
#include "ui/screens/menu_screen.hpp"
#include "ui/ui_session.hpp"
#include "ui/ui_session_app.hpp"

class hello_world_app : public ui::ui_session_app<hello_world_app> {
  ui::screens::menu_screen::actions_vector_t main_menu;
  ui::screens::menu_screen::actions_vector_t menu_screen_demo_menu;
  ui::actions::radio::group demo_radio_group{ 0, [this](const std::string &key) {
                                               std::cout << "Selected radio button with key: " << key << std::endl;
                                               get_ui_session().render();
                                             } };

public:
  hello_world_app() = default;
  explicit hello_world_app(const app_api_t controller_api) : ui_session_app(controller_api) {}

  void setup(app_api_t controller_api) {
    // Set up menu screen demo menu
    menu_screen_demo_menu.emplace_back(
      std::make_unique<ui::actions::button>(GLYPH_ARROW_BACK " Back", [this] { get_ui_session().pop_screen(); }));
    menu_screen_demo_menu.emplace_back(
      std::make_unique<ui::actions::button>("Button", [] { std::cout << "Selected Option 1" << std::endl; }));
    menu_screen_demo_menu.emplace_back(std::make_unique<ui::actions::label>("Label"));
    menu_screen_demo_menu.emplace_back(std::make_unique<ui::actions::page_break>());
    menu_screen_demo_menu.emplace_back(
      std::make_unique<ui::actions::label>("Long label that spans multiple lines", true));
    menu_screen_demo_menu.emplace_back(std::make_unique<ui::actions::page_break>());
    menu_screen_demo_menu.emplace_back(std::make_unique<ui::actions::toggle>(
      "Checkbox",
      [this](const bool state) {
        std::cout << "Toggle state: " << state << std::endl;
        get_ui_session().render();
      },
      false,
      ui::actions::toggle::display_mode::CHECKBOX));
    menu_screen_demo_menu.emplace_back(std::make_unique<ui::actions::toggle>(
      "Switch",
      [this](const bool state) {
        std::cout << "Switch state: " << state << std::endl;
        get_ui_session().render();
      },
      true,
      ui::actions::toggle::display_mode::SWITCH));
    menu_screen_demo_menu.emplace_back(std::make_unique<ui::actions::page_break>());
    menu_screen_demo_menu.emplace_back(std::make_unique<ui::actions::radio>("Radio 1", "radio1", demo_radio_group));
    menu_screen_demo_menu.emplace_back(std::make_unique<ui::actions::radio>("Radio 2", "radio2", demo_radio_group));
    menu_screen_demo_menu.emplace_back(std::make_unique<ui::actions::page_break>());
    menu_screen_demo_menu.emplace_back(std::make_unique<ui::actions::button>("Disabled Button", [] {}, false));
    menu_screen_demo_menu.emplace_back(
      std::make_unique<
        ui::actions::
          toggle>("Disabled Toggle", [](const bool) {}, false, ui::actions::toggle::display_mode::CHECKBOX, false));
    menu_screen_demo_menu.emplace_back(
      std::make_unique<ui::actions::radio>("Disabled Radio", "disabled_radio", demo_radio_group, false));

    // Set up main menu
    main_menu.emplace_back(std::make_unique<ui::actions::button>(GLYPH_ARROW_BACK " Back", [controller_api] {
      controller_api->goto_main_menu();
    }));
    main_menu.emplace_back(std::make_unique<ui::actions::button>("Menu screen demo", [this] {
      get_ui_session().push_screen(
        std::make_unique<ui::screens::menu_screen>(menu_screen_demo_menu, "Menu Screen Demo"));
    }));
    main_menu.emplace_back(std::make_unique<ui::actions::button>("Symbols demo", [this] {
      get_ui_session().push_screen(std::make_unique<symbols_demo>(get_ui_session()));
    }));

    // Push main menu screen
    get_ui_session().push_screen(std::make_unique<ui::screens::menu_screen>(main_menu, "Hello World"));
  }
};

DECLARE_CPP_APP("Hello World", hello_world_app);
