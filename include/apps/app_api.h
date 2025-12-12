#ifndef APP_API_H
#define APP_API_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#define FONT_NOT_FOUND ((uint16_t) (~0))

#define FN_TYPE(_name, _ret, ...) _ret (*_name)(__VA_ARGS__)
#define DECLARE_FN_TYPE(_name, _ret, ...) typedef FN_TYPE(_name, _ret, __VA_ARGS__)
#define _STRINGIFY(x) #x // NOLINT(*-reserved-identifier)
#define STRINGIFY(x) _STRINGIFY(x)

#define REGISTER_APP_FN register_app
#define APP_DESCRIPTOR app_descriptor
#define REGISTER_APP_FN_NAME STRINGIFY(REGISTER_APP_FN)
#define APP_DESCRIPTOR_NAME STRINGIFY(APP_DESCRIPTOR_NAME)

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct Clay_RenderCommandArray;

typedef struct display_controller_api *app_api_t;
typedef const struct display_controller_api *c_app_api_t;

typedef enum display_mode {
  DISPLAY_MODE_BGR565 = 0,
  DISPLAY_MODE_BW1
} display_mode_t;

typedef struct app_menu_entry {
  uintptr_t id;
  const char *name;
} app_menu_entry_t;

/**
 * App teardown function
 *
 * Called when the app is unloaded.
 *
 * @param userptr The user pointer returned by the setup function
 * @param controller_api The controller API object
 */
DECLARE_FN_TYPE(app_teardown_fn_t, void, void *userptr, app_api_t controller_api);

/**
 * Handle keypress event
 *
 * Called by the display controller when a key is pressed while this app's is active. It doesn't need to be provided
 * if the app doesn't render any UI. Apps that do render UI must implement this function to at least allow
 * navigation back to the main menu.
 *
 * @param userptr The user pointer returned by the setup function
 * @param controller_api The controller API object
 * @param button The button that was pressed
 */
DECLARE_FN_TYPE(app_on_keypress_fn_t, void, void *userptr, app_api_t controller_api, int button);

/**
 * Handle app focus event
 *
 * Called by the display controller when this app becomes active. The app should start rendering itself.
 * If not provided, the controller assumes this is a "back-end" app that does not render any UI.
 *
 * @param userptr The user pointer returned by the setup function
 * @param controller_api The controller API object
 */
DECLARE_FN_TYPE(app_on_focus_fn_t, void, void *userptr, app_api_t controller_api);

/**
 * Handle app blur event
 *
 * Called by the display controller when this app is no longer active. The app should stop rendering itself.
 * "Back-end" apps that do not render any UI don't need to implement this function as it will never be called.
 *
 * @param userptr The user pointer returned by the setup function
 * @param controller_api The controller API object
 */
DECLARE_FN_TYPE(app_on_blur_fn_t, void, void *userptr, app_api_t controller_api);

typedef struct app_descriptor {
  const char *name;
  app_teardown_fn_t on_teardown;
  app_on_focus_fn_t on_focus;
  app_on_blur_fn_t on_blur;
  app_on_keypress_fn_t on_keypress;
} app_descriptor_t;

/**
 * App setup function
 *
 * Called when the app is loaded. It may return a user pointer that will be passed to the other app functions.
 *
 * @param controller_api The controller API object
 * @return A user pointer for the app instance, or NULL if no user pointer is needed
 */
DECLARE_FN_TYPE(app_setup_fn_t, void *, app_api_t controller_api);

// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _DECLARE_APP_INTERNAL(_register_app_fn_name,                                              \
                                 _descriptor_name,                                                      \
                                 _app_name,                                                          \
                                 _setup_fn,                                                             \
                                 _teardown_fn,                                                          \
                                 ...)                                                                   \
  app_descriptor_t _descriptor_name = { _app_name, _teardown_fn, __VA_ARGS__ };                   \
  EXTERN_C app_descriptor_t *_register_app_fn_name(app_api_t controller_api, void **userptr) { \
    *userptr = _setup_fn(controller_api);                                                               \
    return &_descriptor_name;                                                                           \
  }                                                                                                     \
  typedef void fake_typedef_to_enforce_semicolon_after_macro

/**
 * Declare a app.
 *
 * C apps must use this macro to declare their app.
 *
 * @snippet _app_example_snippet.c Demonstration of a C app using DECLARE_APP
 *
 * @param _app_name The name of the app
 * @param _setup_fn The setup function
 * @param _teardown_fn The teardown function
 * @param _on_focus Optional focus callback
 * @param _on_blur Optional blur callback
 * @param _on_keypress Optional keypress callback
 */
#define DECLARE_APP(_app_name, _setup_fn, _teardown_fn, ...) \
  _DECLARE_APP_INTERNAL(REGISTER_APP_FN, APP_DESCRIPTOR, _app_name, _setup_fn, _teardown_fn, __VA_ARGS__)

/**
 * App loader callback type
 *
 * Called when loading a app that matches the registered filename extension. The app loader should
 * set up/initialize the loaded app, forwarding any user pointer as needed.
 *
 * @param userptr The user pointer provided when registering the loader
 * @param controller_api The controller API object
 * @param app_path The path to the app file
 * @param app_userptr Output parameter to receive the loaded app's user pointer
 * @return A pointer to the app descriptor, or NULL on failure
 */
DECLARE_FN_TYPE(app_loader_callback_fn_t,
                app_descriptor_t *,
                void *userptr,
                app_api_t controller_api,
                const char *app_path,
                void **app_userptr);

/**
 * Get the current display mode
 *
 * @param controller_api The controller API object
 * @return The current display mode
 */
display_mode_t app_api_get_display_mode(c_app_api_t controller_api);

/**
 * Get the screen width in pixels
 *
 * @param controller_api The controller API object
 * @return The screen width in pixels
 */
size_t app_api_get_screen_width(c_app_api_t controller_api);

/**
 * Get the screen height in pixels
 *
 * @param controller_api The controller API object
 * @return The screen height in pixels
 */
size_t app_api_get_screen_height(c_app_api_t controller_api);

/**
 * Get a font ID by name and size
 *
 * @param controller_api The controller API object
 * @param font_name The name of the font
 * @param font_size The size of the font
 * @return The font ID, or FONT_NOT_FOUND if not found
 */
uint16_t app_api_get_font(c_app_api_t controller_api, const char *font_name, int font_size);

/**
 * Render a frame using Clay rendering commands
 *
 * @param controller_api The controller API object
 * @param cmds The Clay rendering commands
 */
void app_api_clay_render(app_api_t controller_api, const Clay_RenderCommandArray *cmds);

/**
 * Draw a raw frame buffer to the screen
 *
 * @param controller_api The controller API object
 * @param buf The frame buffer data
 * @param buf_size The size of the frame buffer in pixels
 */
void app_api_draw_frame(app_api_t controller_api, const uint16_t *buf, size_t buf_size);

/**
 * Return to the main menu, leaving the current app. `on_blur` will be called before returning.
 *
 * @param controller_api The controller API object
 */
void app_api_goto_main_menu(app_api_t controller_api);

/**
 * Report a fatal error to the controller.
 *
 * Renders an error message to the UI and exits the app, calling `on_blur`. If `unload_app` is true, the app
 * will be unloaded after reporting the error, calling its `on_teardown` function, and removing it from the list of
 * loaded apps.
 *
 * @param controller_api The controller API object
 * @param message The error message to display
 * @param unload_app Whether to unload the app after reporting the error
 */
void app_api_fatal_error(app_api_t controller_api, const char *message, bool unload_app);

/**
 * Register a app loader for a specific file extension
 *
 * @param controller_api The controller API object
 * @param file_extension The file extension to register (including the dot, e.g. ".so")
 * @param loader_fn The loader function to call when loading a app with the specified extension
 * @param userptr A user pointer to pass to the loader function
 */
void app_api_register_app_loader(app_api_t controller_api,
                                       const char *file_extension,
                                       app_loader_callback_fn_t loader_fn,
                                       void *userptr);

#ifdef __cplusplus
}
#endif

#endif
