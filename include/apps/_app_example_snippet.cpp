#include "apps/app_api.hpp"

class my_app {
public:
  // Provide one or both constructors; the constructor with the controller_api parameter is preferred if both are
  // provided
  my_app() {
    // Default constructor
  }

  explicit my_app(app_api_t controller_api) {
    // Constructor with controller API
  }

  /**
   * Called when the app gains focus
   *
   * @param controller_api The controller API object
   */
  void on_enter(app_api_t controller_api);

  /**
   * Called when the app loses focus
   *
   * @param controller_api The controller API object
   */
  void on_leave(app_api_t controller_api);

  /**
   * Handle a keypress event
   *
   * @param controller_api The controller API object
   * @param button The button that was pressed
   */
  void on_keypress(app_api_t controller_api, int button);
};

DECLARE_CPP_APP("My App", my_app);
