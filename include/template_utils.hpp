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
