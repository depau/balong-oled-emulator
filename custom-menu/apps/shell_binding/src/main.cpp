#include <cassert>
#include <iostream>
#include <unistd.h>

#include "adapter.hpp"
#include "apps/app_api.h"
#include "apps/app_api.hpp"
#include "debug.h"

class shell_script_binding_app : public binding_app<shell_script_binding_app, shell_script_adapter, ".sh"> {
public:
  shell_script_binding_app() {}

  explicit shell_script_binding_app(app_api_t controller_api) : binding_app(controller_api) {}

  app_descriptor *load_app(const app_api_t controller_api, const char *app_path, void **app_userptr) {
    if (access(app_path, X_OK) != 0) {
      std::cerr << "warning: Shell script app is not executable: " << app_path << std::endl;
      return nullptr;
    }

    std::string script_name(app_path);
    const size_t last_slash = script_name.find_last_of('/');
    if (last_slash != std::string::npos) {
      script_name = script_name.substr(last_slash + 1);
    }

    const size_t dot_sh = script_name.rfind(".sh");
    if (dot_sh != std::string::npos)
      script_name = script_name.substr(0, dot_sh);

    // Trim [0-9]*- prefix
    size_t prefix_end = 0;
    while (prefix_end < script_name.size() && std::isdigit(script_name[prefix_end]))
      prefix_end++;
    if (prefix_end < script_name.size() && script_name[prefix_end] == '-')
      script_name = script_name.substr(prefix_end + 1);

    // Capitalize
    script_name[0] = static_cast<char>(std::toupper(script_name[0]));

    // Replace underscores with spaces
    for (size_t i = 1; i < script_name.size(); i++) {
      if (script_name[i] == '_')
        script_name[i] = ' ';
    }

    // Fallback name
    if (script_name.empty()) {
      script_name = "[unnamed script]";
    }

    auto *adapter = new adapter_t(controller_api, script_name, app_path);
    *app_userptr = static_cast<void *>(adapter);

    debugf("shell_script_binding: loaded script app: %s (%s)\n", script_name.c_str(), app_path);

    return build_descriptor(script_name);
  }
};

DECLARE_CPP_APP("Shell Script App", shell_script_binding_app);
