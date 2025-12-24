#include <iostream>

#include "apps/app_api.hpp"
#include "clay.hpp"
#include "ui/ui_theme.hpp"

class hello_world_app {
public:
  void on_enter(const app_api_t controller_api) { on_keypress(controller_api, 0); }

  void on_keypress(const app_api_t controller_api, int /*button*/) {
    Clay_BeginLayout();
    std::cout << "Rendering menu UI frame\n";
    constexpr Clay_Color COLOR_BG = { 20, 20, 40, 255 };
    constexpr Clay_Color COLOR_HEADER = { 40, 80, 160, 255 };
    constexpr Clay_Color COLOR_CLIPBOX = { 20, 40, 80, 255 };
    constexpr Clay_Color COLOR_BORDER = { 255, 255, 255, 255 };
    constexpr Clay_Color COLOR_TEXT = { 240, 240, 240, 255 };

    const auto headerBorder = (Clay_BorderElementConfig) {
      .color = COLOR_BORDER,
      .width = { 1, 1, 1, 1, 0 },
    };

    auto textCfg = (Clay_TextElementConfig) {
      .textColor = COLOR_TEXT,
      .fontId = 0,
      .fontSize = 12,
      .letterSpacing = 0,
      .lineHeight = 0,
      .wrapMode = CLAY_TEXT_WRAP_WORDS,
      .textAlignment = CLAY_TEXT_ALIGN_LEFT,
    };

    CLAY({
      .id = CLAY_ID("Root"),
        .layout = {
            .sizing = { CLAY_SIZING_FIXED(128), CLAY_SIZING_FIXED(128) },
            .padding = CLAY_PADDING_ALL(4),
            .childGap = 4,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = COLOR_BG, // RECTANGLE
    }) {
      // Header with border + hello text
      CLAY({
        .id = CLAY_ID("Header"),
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(20) },
            .padding = CLAY_PADDING_ALL(2),
        },
        .backgroundColor = COLOR_HEADER, // RECTANGLE
        .border = headerBorder,          // BORDER
      }) {
        const auto hello = CLAY_STRING("Hello, world!");
        CLAY_TEXT(hello, &textCfg); // TEXT
      }

      // Clipped box with long text to exercise scissor
      CLAY({
        .id = CLAY_ID("ClippedBox"),
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(40) },
            .padding = CLAY_PADDING_ALL(2),
        },
        .backgroundColor = COLOR_CLIPBOX,          // RECTANGLE
        .clip = { .horizontal = false, .vertical = true }, // SCISSOR_START/END
    }) {
        const auto longText = CLAY_STRING("This is long text inside a clipped region. "
                                          "Only the part within the box is visible.");
        CLAY_TEXT(longText, &textCfg); // TEXT + SCISSOR
      }

      // Extra body text
      const auto body = CLAY_STRING("Body: Clay on 128x128/128x64 framebuffers.");
      CLAY_TEXT(body, &textCfg); // TEXT
    }

    controller_api->clay_render(Clay_EndLayout());
  }
};

DECLARE_CPP_APP("Hello World porcodio", hello_world_app);
