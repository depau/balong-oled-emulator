#pragma once

#include <cassert>
#include <functional>
#include <string>

#include "iaction.hpp"
#include "symbols.h"

namespace ui::actions {

/**
 * A radio button action that is part of a group of mutually exclusive options.
 *
 * A radio button must belong to a group. It is assigned an index in the group upon creation.
 * The index will not change during the lifetime of the radio button, including if other radio buttons
 * before it in the group are removed.
 */
class radio final : public iaction {
public:
  /**
   * Callback type for when a radio button is selected.
   *
   * The callback receives the index of the selected radio button in the group.
   *
   * @param key The key of the selected radio button.
   */
  using on_select_fn_t = std::function<void(const std::string &key)>;
  class group;

private:
  std::string text = "Radio button";
  mutable std::string cached_text = "";
  std::string key = "";
  bool enabled = false;
  size_t self_index = -1;
  group *radio_group;

public:
  radio() = default;

  explicit radio(const std::string &text, const std::string &key, group &radio_group, const bool enabled = true) :
    text(text), key(key), enabled(enabled), radio_group(&radio_group) {
    self_index = radio_group.add_to_group(*this);
  }

  ~radio() override { radio_group->remove_from_group(self_index); }

  // Allow move
  radio &operator=(radio &&other) noexcept {
    if (this != &other) {
      text = std::move(other.text);
      enabled = other.enabled;
      radio_group = other.radio_group;
      self_index = other.self_index;

      if (radio_group)
        radio_group->update_ref(self_index, *this);
      other.self_index = ~0;
    }
    return *this;
  }
  radio(radio &&other) noexcept { *this = std::move(other); }

  // Forbid copy
  radio &operator=(const radio &other) = delete;
  radio(const radio &other) = delete;

  const std::string &get_text() const override {
    if (is_checked())
      cached_text = GLYPH_RADIO_BUTTON_CHECKED " " + text;
    else
      cached_text = GLYPH_RADIO_BUTTON_UNCHECKED " " + text;
    return cached_text;
  }

  bool is_selectable() const override { return true; }

  bool is_enabled() const override { return enabled; }

  /**
   * Return whether the radio button is currently checked.
   *
   * @return True if the radio button is checked, false otherwise.
   */
  bool is_checked() const { return &radio_group->get_selected_radio() == this; }

  /**
   * Get the key of the radio button.
   *
   * @return The key of the radio button.
   */
  const std::string &get_key() const { return key; }

  void select() override {
    assert(radio_group && "radio group is nullptr");
    radio_group->select(self_index);
  }

  /**
   * A group of radio buttons that are mutually exclusive.
   */
  class group {
    friend class radio;
    size_t selected_index = ~0;
    std::vector<radio *> radios{};
    on_select_fn_t on_select;

    size_t add_to_group(radio &r) {
#ifdef DEBUG
      for (const radio *existing_radio : radios) {
        if (existing_radio == nullptr)
          continue;
        assert(existing_radio->get_key() != r.get_key() && "Duplicate radio button key in group");
      }
#endif
      radios.push_back(&r);
      return radios.size() - 1;
    }

    void remove_from_group(const size_t index) {
      if (index >= radios.size())
        return;
      // Just blank the entry, to avoid shifting indices of other radios
      radios[index] = nullptr;
    }

    void update_ref(const size_t index, radio &r) {
      if (index >= radios.size())
        return;
      radios[index] = &r;
    }

  public:
    group() = default;

    explicit group(const size_t initial_index, on_select_fn_t &&on_select) :
      selected_index(initial_index), on_select(std::move(on_select)) {}

    // Forbid copy
    group &operator=(const group &other) = delete;
    group(const group &other) = delete;

    // Allow move
    group &operator=(group &&other) noexcept {
      if (this != &other) {
        selected_index = other.selected_index;
        radios = std::move(other.radios);
        on_select = std::move(other.on_select);
        other.selected_index = ~0;

        for (radio *r : radios) {
          if (r != nullptr)
            r->radio_group = this;
        }
      }
      return *this;
    }
    group(group &&other) noexcept { *this = std::move(other); }

    /**
     * Select the radio button at the given index, unchecking all others.
     *
     * @param index The index of the radio button to select.
     */
    void select(const size_t index) {
      assert(index < radios.size() && "Index out of bounds in radio group select");
      if (selected_index == index)
        return;
      selected_index = index;
      for (size_t i = 0; i < radios.size(); ++i) {
        if (radios[i] == nullptr)
          continue;
        const bool cur_checked = i == selected_index;
        if (cur_checked && on_select) {
          on_select(radios[i]->key);
        }
      }
    }

    /**
     * Get the currently selected radio button.
     *
     * @return The currently selected radio button.
     */
    const radio &get_selected_radio() const { return get_radio(selected_index); }

    /**
     * Get the radio button at the given index.
     *
     * @param index The index of the radio button to get.
     * @return The radio button at the given index.
     */
    const radio &get_radio(const size_t index) const {
      assert(index < radios.size() && "Index out of bounds in radio group get_radio");
      assert(radios[index] != nullptr && "Radio is nullptr in radio group get_radio");
      return *radios[index];
    }
  };
};

} // namespace ui::actions