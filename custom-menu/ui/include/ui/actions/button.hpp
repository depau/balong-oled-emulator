#pragma once

#include <functional>
#include <string>

#include "iaction.hpp"

namespace ui::actions {

/**
 * A simple button action that can be selected to perform a callback.
 */
class button final : public iaction {
public:
  using on_select_fn_t = std::function<void()>;

private:
  std::string text = "Button";
  bool enabled = true;
  on_select_fn_t on_select;

public:
  button() = default;
  explicit button(const std::string &text, on_select_fn_t &&on_select, const bool enabled = true) :
    text(text), enabled(enabled), on_select(std::move(on_select)) {}

  const std::string &get_text() const override { return text; }

  bool is_selectable() const override { return true; }

  bool is_enabled() const override { return enabled; }

  void select() override {
    if (on_select) {
      on_select();
    }
  }
};

} // namespace ui::actions