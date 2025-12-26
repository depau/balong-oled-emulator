#pragma once

#include <algorithm>
#include <cstddef>

/**
 * Helper struct to hold a string literal as a template parameter
 *
 * @tparam N The size of the string literal
 */
template<size_t N>
struct StringLiteral {
  // ReSharper disable once CppNonExplicitConvertingConstructor
  constexpr StringLiteral(const char (&str)[N]) {
    std::copy_n(str, N, value);
    value[N] = '\0';
  }

  char value[N + 1];
};

template<typename Parent, typename Child>
constexpr bool is_a(Child child) {
  if constexpr (std::is_pointer_v<Child>) {
    if constexpr (std::is_pointer_v<Parent>)
      return dynamic_cast<Parent>(child) != nullptr;
    return dynamic_cast<const Parent *>(child) != nullptr;
  } else {
    if constexpr (std::is_pointer_v<Parent>)
      return dynamic_cast<Parent>(&child) != nullptr;
    return dynamic_cast<const Parent *>(&child) != nullptr;
  }
}
