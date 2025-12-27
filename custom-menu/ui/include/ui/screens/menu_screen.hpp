#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "clay.hpp"
#include "hooked_functions.h"
#include "ui/actions/button.hpp"
#include "ui/actions/iaction.hpp"
#include "ui/actions/page_break.hpp"
#include "ui/actions/radio.hpp"
#include "ui/actions/toggle.hpp"
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
  const std::string title = "Menu";

  static float get_entry_y_offset(const size_t entry_index, const size_t count_from_entry = 0) {
    assert(entry_index >= count_from_entry);
    float offset = 0.0f;
    for (size_t i = count_from_entry; i < entry_index; i++) {
      const auto [boundingBox, found] = Clay_GetElementData(CLAY_IDI("MenuEntry", static_cast<uint32_t>(i)));
      assert(found);
      offset += boundingBox.height;
    }
    return offset;
  }

  void layout(display_controller_api &controller_api,
              Clay_TextElementConfig &textCfg,
              Clay_TextElementConfig &activeTextCfg,
              Clay_TextElementConfig &disabledTextCfg,
              Clay_TextElementConfig &titleTextCfg,
              const Clay_BorderElementConfig &activeBorderCfg,
              const Clay_BorderElementConfig &disabledActiveBorderCfg,
              const bool can_scroll_up = true) const {
    ROOT_ELEMENT(&controller_api, CLAY_TOP_TO_BOTTOM) {
      if (controller_api.get_screen_height() > 64) {
        ui_add_header(&controller_api, title, &titleTextCfg, can_scroll_up, active_entry < actions->size() - 1);
      }

      CLAY(CLAY_ID("ScrollLayout"), {
        .layout = {
          .sizing = { CLAY_SIZING_GROW(), CLAY_SIZING_GROW() },
          .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .clip = {
          .horizontal = false,
          .vertical = true,
          .childOffset = Clay_GetScrollOffset()
        },
      }) {
        size_t index = 0;
        size_t last_page_break_index = 0;
        for (const auto &action : *actions) {
          Clay_ElementDeclaration element_config;
          Clay_ElementId element_id;
          Clay_TextElementConfig *text_config;

          if (index == active_entry) {
            element_id = CLAY_ID("ActiveMenuEntry");
          } else {
            element_id = CLAY_IDI("MenuEntry", static_cast<uint32_t>(index));
          }

          if (index == active_entry && action->is_hoverable()) {
            element_config = {
              .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .padding = CLAY_PADDING_ALL(MENUENTRY_PADDING),
              },
              .backgroundColor = (action->is_enabled() && action->is_selectable()) ? theme::COLOR_ACTIVE_BACKGROUND : theme::COLOR_DISABLED_ACTIVE_BACKGROUND,
              .clip = { .horizontal =  !action->is_multiline(), .vertical = false },
              .border = (action->is_enabled() && action->is_selectable()) ? activeBorderCfg : disabledActiveBorderCfg,
            };
            text_config = action->is_enabled() ? &activeTextCfg : &disabledTextCfg;
          } else {
            element_config ={
              .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIT(0) },
                .padding = CLAY_PADDING_ALL(MENUENTRY_PADDING),
              },
              .backgroundColor = action->is_enabled() ? theme::COLOR_BACKGROUND : theme::COLOR_DISABLED_BACKGROUND,
              .clip = { .horizontal =  !action->is_multiline(), .vertical = false },
            };
            text_config = action->is_enabled() ? &textCfg : &disabledTextCfg;
          }

          if (is_a<actions::page_break>(&*action)) {
            const float y_offset = get_entry_y_offset(index, last_page_break_index + 1);
            const Clay_ScrollContainerData scroll_info = Clay_GetScrollContainerData(CLAY_ID("ScrollLayout"));
            assert(scroll_info.found);
            const float scroll_container_height = scroll_info.scrollContainerDimensions.height;
            if (y_offset < scroll_container_height) {
              element_config.layout.sizing.height = CLAY_SIZING_FIXED(scroll_container_height - y_offset);
            } else {
              element_config.layout.sizing.height = CLAY_SIZING_FIXED(0);
            }

            last_page_break_index = index;
          }

          CLAY(element_id, element_config) {
            CLAY_TEXT(to_clay_string(action->get_text()), text_config);
          }
          index++;
        }
      }

      if (controller_api.get_screen_height() > 64) {
        const actions::iaction &active_action = *(*actions)[active_entry];
        ui_add_footer(&titleTextCfg, true, active_action.is_selectable() && active_action.is_enabled());
      }
    }
  }

public:
  menu_screen() = default;

  /**
   * Construct a new menu screen with the given list of options
   *
   * @param actions The menu entries to display
   * @param title The title of the menu
   */
  explicit menu_screen(const actions_vector_t &actions, const std::string &title = "Menu") :
    actions(&actions), title(title) {}

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

    auto disabledTextCfg = (Clay_TextElementConfig) {
      .textColor = ui::theme::COLOR_DISABLED_ACTIVE_TEXT,
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

    auto disabledActiveBorderCfg = (Clay_BorderElementConfig) {
      .color = ui::theme::COLOR_DISABLED_ACTIVE_BORDER,
      .width = { theme::BORDER_DISABLED_ACTIVE_PX,
                 theme::BORDER_DISABLED_ACTIVE_PX,
                 theme::BORDER_DISABLED_ACTIVE_PX,
                 theme::BORDER_DISABLED_ACTIVE_PX,
                 0 },
    };

    // Lay out once to compute scroll positions
    Clay_BeginLayout();
    layout(controller_api,
           textCfg,
           activeTextCfg,
           disabledTextCfg,
           titleTextCfg,
           activeBorderCfg,
           disabledActiveBorderCfg);

    float last_page_break_y_offset = 0.0f;
    for (ssize_t i = active_entry - 1; i >= 0; i--) {
      if (is_a<actions::page_break>((*actions)[i].get())) {
        last_page_break_y_offset = get_entry_y_offset(i + 1);
        break;
      }
    }

    const float active_entry_y_offset_from_top = get_entry_y_offset(active_entry);
    const float active_entry_y_offset_from_page_break = active_entry_y_offset_from_top - last_page_break_y_offset;

    const Clay_ScrollContainerData scroll_info = Clay_GetScrollContainerData(CLAY_ID("ScrollLayout"));
    Clay_ElementData active_entry = Clay_GetElementData(CLAY_ID("ActiveMenuEntry"));
    assert(scroll_info.found);
    assert(active_entry.found);

    bool can_scroll_up = false;
    if (active_entry_y_offset_from_page_break + active_entry.boundingBox.height >
        scroll_info.scrollContainerDimensions.height) {
      scroll_info.scrollPosition->y = -static_cast<int>(active_entry_y_offset_from_page_break +
                                                        last_page_break_y_offset + active_entry.boundingBox.height -
                                                        scroll_info.scrollContainerDimensions.height);
      can_scroll_up = true;
    } else {
      scroll_info.scrollPosition->y = -static_cast<int>(last_page_break_y_offset);
      can_scroll_up = last_page_break_y_offset > 0.0f;
    }
    Clay_EndLayout();

    // Now lay out again with the correct scroll position
    Clay_BeginLayout();

    layout(controller_api,
           textCfg,
           activeTextCfg,
           disabledTextCfg,
           titleTextCfg,
           activeBorderCfg,
           disabledActiveBorderCfg,
           can_scroll_up);
    controller_api.clay_render(Clay_EndLayout());

    active_entry = Clay_GetElementData(CLAY_ID("ActiveMenuEntry"));
    assert(active_entry.found);
  }

  void handle_keypress(display_controller_api &controller_api, int button) override {
    switch (button) {
    case BUTTON_MENU: {
      const auto original_entry = active_entry;
      do {
        active_entry = (active_entry + 1) % actions->size();
      } while (!actions->at(active_entry)->is_hoverable() && active_entry != original_entry);
      render(controller_api);
      break;
    }
    case BUTTON_POWER: {
      const auto &action = actions->at(active_entry);
      if (action->is_selectable() && action->is_enabled())
        action->select();
      break;
    }
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
