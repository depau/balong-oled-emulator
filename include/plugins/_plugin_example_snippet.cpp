#include "plugin_api.hpp"
#include "plugins/plugin_api.h"

class my_plugin {
public:
  // Provide one or both constructors; the constructor with the controller_api parameter is preferred if both are
  // provided
  my_plugin() {
    // Default constructor
  }

  explicit my_plugin(plugin_api_t controller_api) {
    // Constructor with controller API
  }

  /**
   * Called when the plugin gains focus
   *
   * @param controller_api The controller API object
   */
  void on_focus(plugin_api_t controller_api);

  /**
   * Called when the plugin loses focus
   *
   * @param controller_api The controller API object
   */
  void on_blur(plugin_api_t controller_api);

  /**
   * Handle a keypress event
   *
   * @param controller_api The controller API object
   * @param button The button that was pressed
   */
  void on_keypress(plugin_api_t controller_api, int button);
};

DECLARE_CPP_PLUGIN("My Plugin", my_plugin);
