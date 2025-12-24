#pragma once

#include "clay.hpp"
#include "ui/ui_theme.hpp"

#define ROOT_ELEMENT(_ctrl, _dir) \
  CLAY({                                                                      \
    .id = CLAY_ID("Root"),                                                    \
      .layout = {                                                             \
        .sizing = {                                                           \
          CLAY_SIZING_FIXED(static_cast<float>((_ctrl)->get_screen_width())), \
          CLAY_SIZING_FIXED(static_cast<float>((_ctrl)->get_screen_height())) \
        },                                                                    \
        .padding = CLAY_PADDING_ALL(ROOT_PADDING),                            \
        .childGap = ROOT_PADDING,                                             \
        .layoutDirection = _dir,                                              \
    },                                                                        \
    .backgroundColor = theme::COLOR_SURFACE,                                  \
  })

#define BOUNDING_BOX \
  CLAY({ \
    .layout = { \
      .sizing = { CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0) }, \
    }, \
    .backgroundColor = (Clay_Color) { 0, 0, 0, 0 }, \
    .border = (Clay_BorderElementConfig) { \
      .color = (Clay_Color) { 255, 0, 0, 255 }, \
      .width = { theme::BORDER_ACTIVE_PX, \
                 theme::BORDER_ACTIVE_PX, \
                 theme::BORDER_ACTIVE_PX, \
                 theme::BORDER_ACTIVE_PX, \
                 0 }, \
    } \
  })
