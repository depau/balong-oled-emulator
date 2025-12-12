#include <dlfcn.h>
#include <iostream>
#include <ostream>

#include "display_controller.hpp"
#include "so_plugin_loader.hpp"

plugin_descriptor *
load_plugin_shared_object(void *, plugin_api_t controller_api, const char *plugin_path, void **plugin_userptr) {
  void *handle = dlopen(plugin_path, RTLD_NOW | RTLD_LOCAL);
  if (!handle) {
    std::cerr << "Failed to load plugin shared object: " << dlerror() << std::endl;
    return nullptr;
  }
  dlerror(); // clear

  const auto reg_fn = reinterpret_cast<plugin_register_fn_t>(dlsym(handle, REGISTER_PLUGIN_FN_NAME));
  if (const char *dlsym_err = dlerror(); dlsym_err != nullptr || !reg_fn) {
    dlclose(handle);
    std::cerr << "Failed to find plugin register function: " << (dlsym_err ? dlsym_err : "symbol is null") << std::endl;
    return nullptr;
  }

  plugin_descriptor *descriptor = reg_fn(controller_api, plugin_userptr);
  if (!descriptor) {
    dlclose(handle);
    std::cerr << "Plugin register function returned null descriptor" << std::endl;
    return nullptr;
  }

  return descriptor;
}
