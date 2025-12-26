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
  ui::ui_session session;

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
    session.render();
  }

  void on_keypress(const app_api_t, const int button) const { session.handle_keypress(button); }

protected:
  ui::ui_session &get_ui_session() { return session; }
};
} // namespace ui