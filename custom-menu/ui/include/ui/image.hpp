#pragma once

#include "clay.hpp"

#define UI_CLAY_IMAGE_DATA(_desc) ((void *) (&_desc))
#define UI_CLAY_IMAGE_SIZING(_desc) \
  { .width = CLAY_SIZING_FIXED((float) _desc.width), .height = CLAY_SIZING_FIXED((float) _desc.height) }
