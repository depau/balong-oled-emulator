#pragma once

#include <cassert>
#include <cstring>
#include <optional>
#include <string>
#include <unistd.h>

#include "apps/app_api.h"
#include "apps/app_api.hpp"
#include "debug.h"
#include "subprocess.hpp"
#include "ui/screens/loading_screen.hpp"
#include "ui/ui_session_app.hpp"

class shell_script_adapter : public ui::ui_session_app<shell_script_adapter> {
  static constexpr std::chrono::steady_clock::duration TIMEOUT = 5000 * std::chrono::milliseconds(1);

  bool entered = false;
  const std::string shell_path = "";
  app_api_t controller_api;
  const std::string orig_title = "Shell Script";
  std::string title = orig_title;
  ui::screens::menu_screen::actions_vector_t actions{};

  std::chrono::steady_clock::time_point process_start_time{};
  uint32_t poll_timer_id = 0;
  bool loading = false;

  subprocess::process process{};

  void show_loading_screen() {
    if (!loading) {
      get_ui_session().replace_screen(std::make_unique<ui::screens::loading_screen>());
      loading = true;
    }
  }

  void run_process(const std::optional<std::string> &arg = std::nullopt);

  void on_action(const std::string &arg) { run_process(arg); }

  void add_deferred_items(std::vector<std::string> &deferred_items);

  void parse_actions_from_stdout(const std::string &stdout_data);

  void poll_process();

  void shutdown_process();

public:
  shell_script_adapter() = default;

  explicit shell_script_adapter(const app_api_t controller_api, const std::string &title, const std::string &path) :
    ui_session_app(controller_api), shell_path(path), controller_api(controller_api), orig_title(title), title(title) {
    assert(controller_api != nullptr);
  }

  void setup(const app_api_t) { show_loading_screen(); }

  void on_enter(const app_api_t) {
    debugf("shell_script_binding: entering %s app\n", shell_path.c_str());
    ui_session_app::on_enter(controller_api);
    if (!entered && !process.is_alive()) {
      run_process();
      show_loading_screen();
    }
    entered = true;
  }

  void on_leave(const app_api_t) {
    debugf("shell_script_binding: leaving %s app\n", shell_path.c_str());
    ui_session_app::on_leave(controller_api);
    shutdown_process();
    entered = false;
  }
};
