#pragma once

#include "apps/app_api.hpp"
#include "sdk19compat.hpp"
#include "ui/ui_session.hpp"

namespace ui {
template<typename T>
concept HasSetup = requires(T &t, app_api_t controller_api) {
  { t.setup(controller_api) } -> sdk19compat::same_as<void>;
};

template<typename Child>
class ui_session_app {
  bool initialized = false;
  ui_session session;

public:
  ui_session_app() = default;
  explicit ui_session_app(const app_api_t controller_api) : session(*controller_api) {}

  void on_enter(app_api_t controller_api) {
    if (!initialized) {
      initialized = true;
      if constexpr (HasSetup<Child>) {
        static_cast<Child *>(this)->setup(controller_api);
      }
    }
    session.on_enter();
  }

  void on_leave(const app_api_t) { session.on_leave(); }

  void on_keypress(const app_api_t, const int button) { session.handle_keypress(button); }

protected:
  ui_session &get_ui_session() { return session; }
};
} // namespace ui