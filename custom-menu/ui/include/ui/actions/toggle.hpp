#pragma once

#include <functional>
#include <string>

#include "iaction.hpp"
#include "symbols.h"

namespace ui::actions {

class toggle final : public iaction {
public:
  using on_select_fn_t = std::function<void(bool)>;

  /**
   * The display mode of the toggle.
   */
  enum class display_mode {
    /**
     * Display the toggle as a checkbox.
     */
    CHECKBOX,

    /**
     * Display the toggle as a switch.
     */
    SWITCH,

    /**
     * Display the toggle as a radio button (this is purely cosmetic and does not enforce radio button behavior).
     */
    RADIO_BUTTON,
  };

private:
  std::string text = "Toggle";
  mutable std::string cached_text = "";
  bool enabled = true;
  bool checked = false;
  display_mode mode = display_mode::CHECKBOX;
  on_select_fn_t on_select;

public:
  toggle() = default;
  explicit toggle(const std::string &text,
                  on_select_fn_t &&on_select,
                  const bool checked,
                  const display_mode mode = display_mode::CHECKBOX,
                  const bool enabled = true) :
    text(text), enabled(enabled), checked(checked), mode(mode), on_select(std::move(on_select)) {}

  const std::string &get_text() const override {
    if (mode == display_mode::SWITCH) {
      if (checked)
        cached_text = GLYPH_TOGGLE_ON " " + text;
      else
        cached_text = GLYPH_TOGGLE_OFF " " + text;
    } else if (mode == display_mode::CHECKBOX) {
      if (checked)
        cached_text = GLYPH_CHECKBOX_CHECKED " " + text;
      else
        cached_text = GLYPH_CHECKBOX_UNCHECKED " " + text;
    } else if (mode == display_mode::RADIO_BUTTON) {
      if (checked)
        cached_text = GLYPH_RADIO_BUTTON_CHECKED " " + text;
      else
        cached_text = GLYPH_RADIO_BUTTON_UNCHECKED " " + text;
    } else {
      cached_text = text;
    }
    return cached_text;
  }

  bool is_selectable() const override { return true; }

  bool is_enabled() const override { return enabled; }

  /**
   * Return whether the toggle is currently checked.
   *
   * @return True if the toggle is checked, false otherwise.
   */
  virtual bool is_checked() const { return checked; }

  /**
   * Get the display mode of the toggle.
   *
   * @return The display mode of the toggle.
   */
  virtual display_mode get_display_mode() const { return mode; }

  void select() override {
    checked = !checked;
    if (on_select) {
      on_select(checked);
    }
  }
};

} // namespace ui::actions
