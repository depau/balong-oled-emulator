#pragma once

#include <algorithm>
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

  void layout(display_controller_api &controller_api,
              Clay_TextElementConfig &textCfg,
              Clay_TextElementConfig &activeTextCfg,
              Clay_TextElementConfig &titleTextCfg,
              const Clay_BorderElementConfig &activeBorderCfg) const {
    ROOT_ELEMENT(&controller_api, CLAY_TOP_TO_BOTTOM) {
      if (controller_api.get_screen_height() > 64) {
        ui_add_header(&controller_api,
                      "Main menu",
                      &titleTextCfg,
                      active_entry > 0,
                      active_entry < actions->size() - 1);
      }

      CLAY({
        .id = CLAY_ID("ScrollLayout"),
        .layout = {
          .sizing = { CLAY_SIZING_GROW(), CLAY_SIZING_GROW() },
          .childGap = ROOT_PADDING,
          .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .clip = {
          .horizontal = false,
          .vertical = true,
          .childOffset = Clay_GetScrollOffset()
        },
      }) {
        size_t index = 0;
        for (const auto &action : *actions) {
          debugf("menu entry: %s\n", action->get_text().c_str());

          if (index == active_entry) {
            CLAY({
              .id = CLAY_ID("ActiveMenuEntry"),
              .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .padding = CLAY_PADDING_ALL(MENUENTRY_PADDING),
              },
              .backgroundColor = theme::COLOR_ACTIVE_BACKGROUND,
              .clip = { .horizontal =  true, .vertical = false },
              .border = activeBorderCfg,
            }) {
              CLAY_TEXT(to_clay_string(action->get_text()), &activeTextCfg);
            }
          } else {
            CLAY({
              .id = CLAY_IDI("MenuEntry", static_cast<uint32_t>(index)),
              .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .padding = CLAY_PADDING_ALL(MENUENTRY_PADDING),
              },
              .backgroundColor = theme::COLOR_BACKGROUND,
              .clip = { .horizontal =  true, .vertical = false },
            }) {
              CLAY_TEXT(to_clay_string(action->get_text()), &textCfg);
            }
          }
          index++;
        }
      }

      if (controller_api.get_screen_height() > 64) {
        ui_add_footer(&titleTextCfg, true, true);
      }
    }
  }

public:
  menu_screen() = default;

  /**
   * Construct a new menu screen with the given list of options
   *
   * @param actions The menu entries to display
   */
  explicit menu_screen(const actions_vector_t &actions) : actions(&actions) {}

  void render(display_controller_api &controller_api) override {
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

    // Lay out once to compute scroll positions
    Clay_BeginLayout();
    layout(controller_api, textCfg, activeTextCfg, titleTextCfg, activeBorderCfg);

    float active_entry_offset_from_top = 0.0f;
    for (size_t i = 0; i < active_entry; i++) {
      const auto [boundingBox, found] = Clay_GetElementData(CLAY_IDI("MenuEntry", static_cast<uint32_t>(i)));
      assert(found);
      active_entry_offset_from_top += boundingBox.height + ROOT_PADDING;
    }

    const Clay_ScrollContainerData scroll_info = Clay_GetScrollContainerData(CLAY_ID("ScrollLayout"));
    const Clay_ElementData active_entry_info = Clay_GetElementData(CLAY_ID("ActiveMenuEntry"));
    assert(scroll_info.found);
    assert(active_entry_info.found);

    if (active_entry_offset_from_top + active_entry_info.boundingBox.height >
        scroll_info.scrollContainerDimensions.height) {
      scroll_info.scrollPosition->y = -active_entry_offset_from_top - active_entry_info.boundingBox.height +
                                      scroll_info.scrollContainerDimensions.height;
    } else {
      scroll_info.scrollPosition->y = 0;
    }
    Clay_EndLayout();

    // Now lay out again with the correct scroll position
    Clay_BeginLayout();
    layout(controller_api, textCfg, activeTextCfg, titleTextCfg, activeBorderCfg);
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
