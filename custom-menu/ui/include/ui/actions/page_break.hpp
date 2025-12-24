#pragma once

#include "iaction.hpp"

namespace ui::actions {

/**
 * A marker to indicate that a page break should be rendered in the UI.
 */
class page_break final : public iaction {
public:
  page_break() = default;

  bool is_selectable() const override { return false; }

  bool is_enabled() const override { return false; }
};

} // namespace ui::actions
