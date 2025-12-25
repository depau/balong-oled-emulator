#pragma once
#include <type_traits>

// The Android SDK 19 does not support std::same_as, so we reimplement it here
namespace sdk19compat {
template<typename _Tp, typename _Up>
concept __same_as = std::is_same_v<_Tp, _Up>;

template<typename _Tp, typename _Up>
concept same_as = __same_as<_Tp, _Up> && __same_as<_Up, _Tp>;

template<typename _Derived, typename _Base>
concept derived_from = __is_base_of(_Base, _Derived) &&
                       std::is_convertible_v<const volatile _Derived *, const volatile _Base *>;
} // namespace sdk19compat
