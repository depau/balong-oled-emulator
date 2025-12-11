#ifndef PLUGIN_API_H
#define PLUGIN_API_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#define FONT_NOT_FOUND ((uint16_t) (~0))

#define FN_TYPE(_name, _ret, ...) _ret (*_name)(__VA_ARGS__)
#define DECLARE_FN_TYPE(_name, _ret, ...) typedef FN_TYPE(_name, _ret, __VA_ARGS__)

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "clay.h"

typedef struct display_controller_api *plugin_api_t;
typedef const struct display_controller_api *c_plugin_api_t;

typedef enum display_mode {
  DISPLAY_MODE_BGR565 = 0,
  DISPLAY_MODE_BW1
} display_mode_t;

typedef struct plugin_menu_entry {
  uintptr_t id;
  const char *name;
} plugin_menu_entry_t;

/**
 * Plug-in teardown function
 *
 * Called when the plugin is unloaded.
 *
 * @param userptr The user pointer returned by the setup function
 * @param controller_api The controller API object
 */
DECLARE_FN_TYPE(plugin_teardown_fn_t, void, void *userptr, plugin_api_t controller_api);

/**
 * Handle keypress event
 *
 * Called by the display controller when a key is pressed while this plug-in's is active. It doesn't need to be provided
 * if the plug-in doesn't render any UI. Plug-ins that do render UI must implement this function to at least allow
 * navigation back to the main menu.
 *
 * @param userptr The user pointer returned by the setup function
 * @param controller_api The controller API object
 * @param button The button that was pressed
 */
DECLARE_FN_TYPE(plugin_on_keypress_fn_t, void, void *userptr, plugin_api_t controller_api, int button);

/**
 * Handle plug-in focus event
 *
 * Called by the display controller when this plug-in becomes active. The plug-in should start rendering itself.
 * If not provided, the controller assumes this is a "back-end" plug-in that does not render any UI.
 *
 * @param userptr The user pointer returned by the setup function
 * @param controller_api The controller API object
 */
DECLARE_FN_TYPE(plugin_on_focus_fn_t, void, void *userptr, plugin_api_t controller_api);

/**
 * Handle plug-in blur event
 *
 * Called by the display controller when this plug-in is no longer active. The plug-in should stop rendering itself.
 * "Back-end" plug-ins that do not render any UI don't need to implement this function as it will never be called.
 *
 * @param userptr The user pointer returned by the setup function
 * @param controller_api The controller API object
 */
DECLARE_FN_TYPE(plugin_on_blur_fn_t, void, void *userptr, plugin_api_t controller_api);

typedef struct plugin_descriptor {
  const char *name;
  plugin_teardown_fn_t on_teardown;
  plugin_on_focus_fn_t on_focus;
  plugin_on_blur_fn_t on_blur;
  plugin_on_keypress_fn_t on_keypress;
} plugin_descriptor_t;

/**
 * Plug-in setup function
 *
 * Called when the plug-in is loaded. It may return a user pointer that will be passed to the other plug-in functions.
 *
 * @param controller_api The controller API object
 * @return A user pointer for the plug-in instance, or NULL if no user pointer is needed
 */
DECLARE_FN_TYPE(plugin_setup_fn_t, void *, plugin_api_t controller_api);

// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _DECLARE_PLUGIN_INTERNAL(_register_plugin_fn_name,                                              \
                                 _descriptor_name,                                                      \
                                 _plugin_name,                                                          \
                                 _setup_fn,                                                             \
                                 _teardown_fn,                                                          \
                                 ...)                                                                   \
  plugin_descriptor_t _descriptor_name = { _plugin_name, _teardown_fn, __VA_ARGS__ };                   \
  EXTERN_C plugin_descriptor_t *_register_plugin_fn_name(plugin_api_t controller_api, void **userptr) { \
    *userptr = _setup_fn(controller_api);                                                               \
    return &_descriptor_name;                                                                           \
  }                                                                                                     \
  typedef void fake_typedef_to_enforce_semicolon_after_macro

/**
 * Declare a plug-in.
 *
 * C/C++ plug-ins must use this macro to declare their plug-in.
 *
 * @param _plugin_name The name of the plug-in
 * @param _setup_fn The setup function
 * @param _teardown_fn The teardown function
 * @param _on_focus Optional focus callback
 * @param _on_blur Optional blur callback
 * @param _on_keypress Optional keypress callback
 */
#define DECLARE_PLUGIN(_plugin_name, _setup_fn, _teardown_fn, ...) \
  _DECLARE_PLUGIN_INTERNAL(register_plugin, plugin_descriptor, _plugin_name, _setup_fn, _teardown_fn, __VA_ARGS__)

/**
 * Plug-in loader callback type
 *
 * Called when loading a plug-in that matches the registered filename extension. The plug-in loader should
 * set up/initialize the loaded plug-in, forwarding any user pointer as needed.
 *
 * @param userptr The user pointer provided when registering the loader
 * @param controller_api The controller API object
 * @param plugin_path The path to the plug-in file
 * @param plugin_userptr Output parameter to receive the loaded plug-in's user pointer
 * @return A pointer to the plug-in descriptor, or NULL on failure
 */
DECLARE_FN_TYPE(plugin_loader_callback_fn_t,
                plugin_descriptor_t *,
                void *userptr,
                plugin_api_t controller_api,
                const char *plugin_path,
                void **plugin_userptr);

/**
 * Get the current display mode
 *
 * @param controller_api The controller API object
 * @return The current display mode
 */
display_mode_t plugin_api_get_display_mode(c_plugin_api_t controller_api);

/**
 * Get the screen width in pixels
 *
 * @param controller_api The controller API object
 * @return The screen width in pixels
 */
size_t plugin_api_get_screen_width(c_plugin_api_t controller_api);

/**
 * Get the screen height in pixels
 *
 * @param controller_api The controller API object
 * @return The screen height in pixels
 */
size_t plugin_api_get_screen_height(c_plugin_api_t controller_api);

/**
 * Get a font ID by name and size
 *
 * @param controller_api The controller API object
 * @param font_name The name of the font
 * @param font_size The size of the font
 * @return The font ID, or FONT_NOT_FOUND if not found
 */
uint16_t plugin_api_get_font(c_plugin_api_t controller_api, const char *font_name, int font_size);

/**
 * Render a frame using Clay rendering commands
 *
 * @param controller_api The controller API object
 * @param cmds The Clay rendering commands
 */
void plugin_api_clay_render(plugin_api_t controller_api, const Clay_RenderCommandArray *cmds);

/**
 * Draw a raw frame buffer to the screen
 *
 * @param controller_api The controller API object
 * @param buf The frame buffer data
 * @param buf_size The size of the frame buffer in pixels
 */
void plugin_api_draw_frame(plugin_api_t controller_api, const uint16_t *buf, size_t buf_size);

/**
 * Return to the main menu, leaving the current plug-in. `on_blur` will be called before returning.
 *
 * @param controller_api The controller API object
 */
void plugin_api_goto_main_menu(plugin_api_t controller_api);

/**
 * Report a fatal error to the controller.
 *
 * Renders an error message to the UI and exits the plug-in, calling `on_blur`. If `unload_plugin` is true, the plug-in
 * will be unloaded after reporting the error, calling its `on_teardown` function, and removing it from the list of
 * loaded plug-ins.
 *
 * @param controller_api The controller API object
 * @param message The error message to display
 * @param unload_plugin Whether to unload the plug-in after reporting the error
 */
void plugin_api_fatal_error(plugin_api_t controller_api, const char *message, bool unload_plugin);

/**
 * Register a plug-in loader for a specific file extension
 *
 * @param controller_api The controller API object
 * @param file_extension The file extension to register (including the dot, e.g. ".so")
 * @param loader_fn The loader function to call when loading a plug-in with the specified extension
 * @param userptr A user pointer to pass to the loader function
 */
void plugin_api_register_plugin_loader(plugin_api_t controller_api,
                                       const char *file_extension,
                                       plugin_loader_callback_fn_t loader_fn,
                                       void *userptr);

#ifdef __cplusplus
}
#endif

#endif
