#pragma once
#include <chrono>

#include "apps/app_api.hpp"

namespace ui::screens {

/**
 * Abstract base class for UI screens.
 */
class iscreen {
public:
  virtual ~iscreen() = default;

  /**
   * Render the screen using the provided display controller API
   *
   * @param controller_api The display controller API to use for rendering
   */
  virtual void render(display_controller_api &controller_api) = 0;

  /**
   * Handle a keypress event. The screen should re-render itself if necessary.
   *
   * @param controller_api The display controller API
   * @param button The button that was pressed
   */
  virtual void handle_keypress(display_controller_api &controller_api, int button) = 0;

  /**
   * Notify how many ticks per second this screen needs, i.e. for animations.
   * The UI session will check this value on startup, at every user interaction, and after each tick.
   *
   * @return The number of ticks per second needed. Return 0 for no ticks.
   */
  [[nodiscard]] virtual int needs_ticks_per_second() const { return 0; }

  /**
   * Tick function called at the rate specified by `needsTicksPerSecond`. The screen should
   * re-render itself if necessary.
   *
   * @param controller_api The display controller API
   * @param now The current time point, as a monotonic steady clock time point
   */
  virtual void tick(display_controller_api &controller_api, std::chrono::steady_clock::time_point now) {}
};

} // namespace ui::screens
