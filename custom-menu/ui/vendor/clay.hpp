#pragma once

#include <string>

extern "C" {
#include "clay.h"
}

inline Clay_String to_clay_string(const std::string &str) {
  return Clay_String{
    .isStaticallyAllocated = false,
    .length = static_cast<int32_t>(str.size()),
    .chars = str.c_str(),
  };
}
