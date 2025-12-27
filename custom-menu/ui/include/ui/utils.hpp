#pragma once

#include "clay.hpp"
#include "symbols.h"
#include "ui/ui_theme.hpp"

#define ROOT_ELEMENT_SIZING(_ctrl)                                      \
  { CLAY_SIZING_FIXED(static_cast<float>((_ctrl)->get_screen_width())), \
    CLAY_SIZING_FIXED(static_cast<float>((_ctrl)->get_screen_height())) }

#define ROOT_ELEMENT(_ctrl, _dir) \
  CLAY(CLAY_ID("Root"), {                          \
      .layout = {                                  \
        .sizing = ROOT_ELEMENT_SIZING(_ctrl),      \
        .padding = CLAY_PADDING_ALL(ROOT_PADDING), \
        .childGap = ROOT_PADDING,                  \
        .layoutDirection = _dir,                   \
    },                                             \
    .backgroundColor = ui::theme::COLOR_SURFACE,   \
  })

#define BOUNDING_BOX \
  CLAY({ \
    .layout = { \
      .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) }, \
    }, \
    .backgroundColor = (Clay_Color) { 0, 0, 0, 0 }, \
    .border = (Clay_BorderElementConfig) { \
      .color = (Clay_Color) { 255, 0, 0, 255 }, \
      .width = { ui::theme::BORDER_ACTIVE_PX, \
                 ui::theme::BORDER_ACTIVE_PX, \
                 ui::theme::BORDER_ACTIVE_PX, \
                 ui::theme::BORDER_ACTIVE_PX, \
                 0 }, \
    } \
  })

inline void ui_horizontal_line(Clay_Color color,
                               const float box_height,
                               const Clay_Padding padding = CLAY_PADDING_ALL(0),
                               const float thickness = 1) {
  CLAY(CLAY_ID("HorizontalLine"), {
    .layout = {
      .sizing = { CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(box_height) },
      .padding = padding,
    },
  }) {
    CLAY(CLAY_ID("HorizontalLineChild"), {
     .layout = {
       .sizing = { CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(thickness) },
     },
     .backgroundColor = ui::theme::COLOR_TEXT,
   }) {
    }
  }
}

inline void ui_add_header(app_api_t controller_api,
                          const std::string &title,
                          Clay_TextElementConfig *textCfg,
                          const bool can_scroll_up = false,
                          const bool can_scroll_down = false) {
  const auto [textWidth, textHeight] = controller_api->clay_measure_text(title, textCfg);

  CLAY(CLAY_ID("Header"), {
    .layout = {
      .sizing = { CLAY_SIZING_GROW(), CLAY_SIZING_FIT() },
      .padding = { .left = 2, .right = 2, .bottom = 2 },
      .layoutDirection = CLAY_LEFT_TO_RIGHT,
    },
  }) {

    ui_horizontal_line(ui::theme::COLOR_TEXT, textHeight, { 0, 4, static_cast<uint16_t>(textHeight / 2), 0 });

    const Clay_String headerText = {
      .isStaticallyAllocated = false,
      .length = static_cast<int32_t>(title.size()),
      .chars = title.c_str(),
    };
    CLAY_TEXT(headerText, textCfg);

    // Left horizontal line
    const bool scroll = can_scroll_up || can_scroll_down;
    const auto [caretWidth, caretHeight] = controller_api->clay_measure_text(GLYPH_CARET_UP, textCfg);
    CLAY(CLAY_ID("HeaderRightLine"), {
      .layout = {
        .sizing = { CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(textHeight) },
        .padding = {4, static_cast<uint16_t>(scroll ? caretWidth - 2 : 0), static_cast<uint16_t>(textHeight / 2), 0},
      },
    }) {
      CLAY(CLAY_ID("HeaderRightLineChild"), {
       .layout = {
         .sizing = { CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(1) },
       },
       .backgroundColor = ui::theme::COLOR_TEXT,
     }) {
      }

      if (can_scroll_up) {
        CLAY(CLAY_ID("CaretUp"), {
          .layout = {
            .sizing = { CLAY_SIZING_FIXED(caretWidth - 2), CLAY_SIZING_FIXED(caretHeight) },
          },
          .floating = {
            .offset = {
              .x = 2,
              .y = -caretHeight/5 + 2,
            },
            .attachPoints = {
              .element = CLAY_ATTACH_POINT_RIGHT_TOP,
              .parent = CLAY_ATTACH_POINT_RIGHT_TOP
            },
            .attachTo = CLAY_ATTACH_TO_PARENT,
          }
        }) {
          CLAY_TEXT(CLAY_STRING_CONST(GLYPH_CARET_UP), textCfg);
        }
      }

      if (can_scroll_down) {
        CLAY(CLAY_ID("CaretDown"), {
          .layout = {
            .sizing = { CLAY_SIZING_FIXED(caretWidth - 2), CLAY_SIZING_FIXED(caretHeight) },
          },
          .floating = {
            .offset = {
              .x = 2,
              .y = caretHeight/5 + 2,
            },
            .attachPoints = {
              .element = CLAY_ATTACH_POINT_RIGHT_TOP,
              .parent = CLAY_ATTACH_POINT_RIGHT_TOP
            },
            .attachTo = CLAY_ATTACH_TO_PARENT
          }
        }) {
          CLAY_TEXT(CLAY_STRING_CONST(GLYPH_CARET_DOWN), textCfg);
        }
      }
    }
  }
}

inline void ui_add_footer(Clay_TextElementConfig *textCfg, const bool can_press_menu, const bool can_press_power) {
  CLAY(CLAY_ID("Footer"), {
    .layout = {
      .sizing = { CLAY_SIZING_GROW(), CLAY_SIZING_FIT() },
      .padding = { .top = ROOT_PADDING },
      .childGap = 4,
      .childAlignment = {
        .x = CLAY_ALIGN_X_CENTER,
        .y = CLAY_ALIGN_Y_CENTER,
      },
      .layoutDirection = CLAY_LEFT_TO_RIGHT,
    },
    .border = {
      .color = ui::theme::COLOR_TEXT,
      .width = { .top = 1 },
    }
  }) {
    if (can_press_menu) {
      CLAY_TEXT(CLAY_STRING_CONST(GLYPH_MENU " Next"), textCfg);
    }
    if (can_press_power) {
      CLAY_TEXT(CLAY_STRING_CONST(GLYPH_POWER_BUTTON " Select"), textCfg);
    }
  }
}