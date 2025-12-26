#pragma once
#include <string>

namespace ui::actions {

inline const std::string empty_string = "";
/**
 * Interface for an action that can be performed in the UI.
 */
class iaction {
public:
  iaction() = default;

  virtual ~iaction() = default;

  /**
   * Get the text of the action.
   *
   * @return The text of the action.
   */
  virtual const std::string &get_text() const { return empty_string; }

  /**
   * Return whether the action can be selected by pressing the selection button.
   *
   * @return True if the action is selectable, false otherwise.
   */
  virtual bool is_selectable() const { return false; }

  /**
   * Return whether the action can be hovered. If false, menus will entirely skip over this action when navigating.
   *
   * @return True if the action is hoverable, false otherwise.
   */
  virtual bool is_hoverable() const { return is_selectable(); }

  /**
   * Return whether the action is enabled, regardless of whether it is selectable.
   * In other words, an action can be selectable but disabled (e.g., a grayed-out menu item).
   *
   * @return True if the action is enabled, false otherwise.
   */
  virtual bool is_enabled() const { return true; }

  /**
   * Perform the action's selection behavior.
   */
  virtual void select() {}
};

} // namespace ui::actions