#pragma once

#include <cassert>

#include "debug.h"
#include "hooked_functions.h"
#include "ui/actions/button.hpp"
#include "ui/screens/iscreen.hpp"
#include "ui/ui_theme.hpp"
#include "ui/utils.hpp"

namespace ui::screens {
class menu_screen final : public iscreen {
public:
  using on_navigate_back_fn_t = actions::button::on_select_fn_t;
  using actions_vector_t = std::vector<std::unique_ptr<actions::iaction>>;

private:
  const actions_vector_t *actions;
  size_t active_entry = 0;

public:
  menu_screen() = default;

  /**
   * Construct a new menu screen with the given list of options
   *
   * @param actions The menu entries to display
   */
  explicit menu_screen(const actions_vector_t &actions) : actions(&actions) {}

  void render(display_controller_api &controller_api) override {
    Clay_BeginLayout();

    auto textCfg = (Clay_TextElementConfig) {
      .textColor = ui::theme::COLOR_TEXT,
      .fontId = controller_api.get_font(theme::FONT_NAME_TEXT, theme::FONT_SIZE_TEXT).value_or(0),
      .fontSize = theme::FONT_SIZE_TEXT,
      .letterSpacing = 0,
      .lineHeight = 0,
      .wrapMode = CLAY_TEXT_WRAP_WORDS,
      .textAlignment = CLAY_TEXT_ALIGN_LEFT,
    };

    auto activeTextCfg = (Clay_TextElementConfig) {
      .textColor = ui::theme::COLOR_ACTIVE_TEXT,
      .fontId = controller_api.get_font(theme::FONT_NAME_TEXT, theme::FONT_SIZE_TEXT).value_or(0),
      .fontSize = theme::FONT_SIZE_TEXT,
      .letterSpacing = 0,
      .lineHeight = 0,
      .wrapMode = CLAY_TEXT_WRAP_WORDS,
      .textAlignment = CLAY_TEXT_ALIGN_LEFT,
    };

    auto titleTextCfg = (Clay_TextElementConfig) {
      .textColor = ui::theme::COLOR_TEXT,
      .fontId = controller_api.get_font(ui::theme::FONT_NAME_TEXT, ui::theme::FONT_SIZE_TEXT_SMALL).value_or(0),
      .fontSize = ui::theme::FONT_SIZE_TEXT_SMALL,
      .wrapMode = CLAY_TEXT_WRAP_WORDS,
      .textAlignment = CLAY_TEXT_ALIGN_CENTER,
    };

    auto activeBorderCfg = (Clay_BorderElementConfig) {
      .color = ui::theme::COLOR_ACTIVE_BORDER,
      .width = { theme::BORDER_ACTIVE_PX,
                 theme::BORDER_ACTIVE_PX,
                 theme::BORDER_ACTIVE_PX,
                 theme::BORDER_ACTIVE_PX,
                 0 },
    };

    ROOT_ELEMENT(&controller_api, CLAY_TOP_TO_BOTTOM) {
      if (controller_api.get_screen_height() > 64) {
        ui_add_header(&controller_api,
                      "Main menu",
                      &titleTextCfg,
                      active_entry > 0,
                      active_entry < actions->size() - 1);
      }

      size_t index = 0;
      for (const auto &action : *actions) {
        debugf("menu entry: %s\n", action->get_text().c_str());

        if (index++ == active_entry) {
          CLAY({
            .layout = {
              .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
              .padding = CLAY_PADDING_ALL(MENUENTRY_PADDING),
            },
            .backgroundColor = theme::COLOR_ACTIVE_BACKGROUND,
            .clip = { .horizontal =  true, .vertical = false },
            .border = activeBorderCfg,
          }) {
            // BOUNDING_BOX {
            CLAY_TEXT(to_clay_string(action->get_text()), &activeTextCfg);
            // }
          }
        } else {
          CLAY({
            .layout = {
              .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
              .padding = CLAY_PADDING_ALL(MENUENTRY_PADDING),
            },
            .backgroundColor = theme::COLOR_BACKGROUND,
            .clip = { .horizontal =  true, .vertical = false },
          }) {
            // BOUNDING_BOX {
            CLAY_TEXT(to_clay_string(action->get_text()), &textCfg);
            // }
          }
        }
      }
    }

    controller_api.clay_render(Clay_EndLayout());
  }

  void handle_keypress(display_controller_api &controller_api, int button) override {
    switch (button) {
    case BUTTON_MENU:
      if (++active_entry >= actions->size())
        active_entry = 0;
      render(controller_api);
      break;
    case BUTTON_POWER:
      actions->at(active_entry)->select();
      break;
    default:
      break;
    }
  }

  void set_active_entry(const size_t entry_index) {
    assert(entry_index < actions->size());
    active_entry = entry_index;
  }
};
} // namespace ui::screens
