#pragma once

#include "clay.hpp"

namespace ui::theme {

// Colors
constexpr Clay_Color COLOR_SURFACE = { 0, 0, 0, 255 };
constexpr Clay_Color COLOR_TEXT = { 255, 255, 255, 255 };
constexpr Clay_Color COLOR_BACKGROUND = { 0, 0, 0, 255 };

constexpr Clay_Color COLOR_ACTIVE_TEXT = COLOR_TEXT;
constexpr Clay_Color COLOR_ACTIVE_BACKGROUND = COLOR_BACKGROUND;
constexpr Clay_Color COLOR_ACTIVE_BORDER = { 255, 255, 255, 255 };

constexpr Clay_Color COLOR_DISABLED_TEXT = { 128, 128, 128, 255 };
constexpr Clay_Color COLOR_DISABLED_BACKGROUND = { 0, 0, 0, 255 };

constexpr Clay_Color COLOR_DISABLED_ACTIVE_TEXT = { 128, 128, 128, 255 };
constexpr Clay_Color COLOR_DISABLED_ACTIVE_BACKGROUND = { 0, 0, 0, 255 };
constexpr Clay_Color COLOR_DISABLED_ACTIVE_BORDER = { 128, 128, 128, 255 };

// Dimensions
constexpr int BORDER_PX = 1;

constexpr int BORDER_ACTIVE_PX = BORDER_PX;
constexpr int BORDER_DISABLED_ACTIVE_PX = BORDER_PX;

#define ROOT_PADDING 4
#define LIST_PADDING 2
#define MENUENTRY_PADDING 2

// Typography
constexpr auto FONT_NAME_TEXT = "Poppins";
constexpr int FONT_SIZE_TEXT = 12;
constexpr int FONT_SIZE_TEXT_SMALL = 8;

} // namespace ui::theme
