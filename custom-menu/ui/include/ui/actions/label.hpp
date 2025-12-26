#pragma once

#include <string>

#include "iaction.hpp"

namespace ui::actions {

/**
 * A label that displays non-interactive text.
 */
class label final : public iaction {
  std::string text = "Button";
  bool multiline = false;

public:
  label() = default;
  explicit label(const std::string &text, const bool multiline = false) : text(text), multiline(multiline) {}

  const std::string &get_text() const override { return text; }

  bool is_selectable() const override { return false; }

  bool is_hoverable() const override { return true; }

  bool is_enabled() const override { return true; }

  bool is_multiline() const override { return multiline; }
};

} // namespace ui::actions