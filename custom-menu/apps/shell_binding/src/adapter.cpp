#include <cassert>
#include <chrono>
#include <cstring>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "adapter.hpp"
#include "debug.h"
#include "ui/actions/label.hpp"

void shell_script_adapter::run_process(const std::optional<std::string> &arg) {
  assert(!process.is_alive());
  title = orig_title;

  std::vector<std::string> argv;
  argv.push_back(shell_path);
  if (arg.has_value())
    argv.push_back(*arg);

  debugf("shell_script_binding: running");
  for (const std::string &arg : argv) {
    debugf(" '%s'", arg.c_str());
  }
  debugf("\n");

  const std::optional<int> res = process.run(argv, true);
  if (res.has_value()) {
    controller_api->fatal_error(std::string("Failed to run script: ") + strerror(*res));
    return;
  }
  process_start_time = std::chrono::steady_clock::now();

  if (poll_timer_id == 0)
    poll_timer_id = controller_api->schedule_timer(100, true, [this] { poll_process(); });
}

void shell_script_adapter::add_deferred_items(std::vector<std::string> &deferred_items) {
  const bool is_radio_group = std::any_of(deferred_items.begin(), deferred_items.end(), [](const std::string &item) {
    return item.starts_with("<") && item.find(">:", 6) != std::string::npos;
  });
  for (const std::string &item : deferred_items) {
    const size_t sep_pos = item.find(':');
    if (sep_pos != std::string::npos) {
      std::string action_text = item.substr(0, sep_pos);
      const std::string action_arg = item.substr(sep_pos + 1);

      if (is_radio_group) {
        bool is_checked = false;
        if (action_text.starts_with('<') && action_text.ends_with('>')) {
          is_checked = true;
          action_text = action_text.substr(1, action_text.size() - 2);
        }

        // Trim
        action_text.erase(0, action_text.find_first_not_of(" \t\r\n"));
        action_text.erase(action_text.find_last_not_of(" \t\r\n") + 1);

        actions.emplace_back(std::make_unique<ui::actions::toggle>(
          action_text,
          [this, action_arg](bool) { on_action(action_arg); },
          is_checked,
          ui::actions::toggle::display_mode::RADIO_BUTTON));
      } else {
        // Trim
        action_text.erase(0, action_text.find_first_not_of(" \t\r\n"));
        action_text.erase(action_text.find_last_not_of(" \t\r\n") + 1);

        actions.emplace_back(
          std::make_unique<ui::actions::button>(action_text, [this, action_arg] { on_action(action_arg); }));
      }
    } else {
      std::cerr << "Invalid item line: item:" << item << std::endl;
    }
  }
}
void shell_script_adapter::parse_actions_from_stdout(const std::string &stdout_data) {
  actions.clear();
  actions.push_back(
    std::make_unique<ui::actions::button>(GLYPH_ARROW_BACK " Back", [this] { controller_api->goto_main_menu(); }));

  std::vector<std::string> deferred_items;
  std::istringstream stream(stdout_data);
  std::string line;

  while (std::getline(stream, line)) {
    if (!deferred_items.empty() && !line.starts_with("item:")) {
      add_deferred_items(deferred_items);
      deferred_items.clear();
    }

    if (line.starts_with("item:")) {
      const std::string rest = line.substr(5);
      deferred_items.push_back(rest);
    } else if (line.starts_with("title:")) {
      title = line.substr(6);
    } else if (line.starts_with("text:")) {
      const std::string action_text = line.substr(5);
      actions.emplace_back(std::make_unique<ui::actions::label>(action_text, true));
    } else if (line.starts_with("pagebreak:")) {
      actions.emplace_back(std::make_unique<ui::actions::page_break>());
    }
  }

  if (!deferred_items.empty())
    add_deferred_items(deferred_items);

  debugf("shell_script_binding: parsed %zu actions\n", actions.size());
}

void shell_script_adapter::poll_process() {
  if (process.is_alive()) {
    show_loading_screen();
    if (std::chrono::steady_clock::now() - process_start_time > TIMEOUT) {
      debugf("shell_script_binding: script timed out\n");
      shutdown_process();
      controller_api->fatal_error("Script timed out");
    }
  } else {
    const std::optional<int> maybe_exit_code = process.get_exit_code();
    assert(maybe_exit_code.has_value());
    const int exit_code = *maybe_exit_code;
    debugf("shell_script_binding: script exited with code %d\n", exit_code);

    if (exit_code != 0)
      return controller_api->fatal_error(std::string("Script failed with code ") + std::to_string(exit_code));

      debugf("shell_script_binding: cancelling poll timer %u\n", poll_timer_id);
      controller_api->cancel_timer(poll_timer_id);
      poll_timer_id = 0;
      debugf("shell_script_binding: timer cancelled\n");

    debugf("shell_script_binding: parsing actions from stdout\n");
    const size_t prev_action_count = actions.size();
    parse_actions_from_stdout(process.get_stdout());

    size_t old_index = 0;
    const auto top_screen = get_ui_session().get_top_screen();
    if (top_screen.has_value()) {
      if (const auto menu_screen = dynamic_cast<ui::screens::menu_screen *>(&top_screen->get())) {
        old_index = menu_screen->get_active_entry();
      }
    }

    auto &new_screen = get_ui_session().replace_screen(std::make_unique<ui::screens::menu_screen>(actions, title));
    loading = false;

    // Heuristic: if the number of actions didn't change, keep the same selection index
    if (prev_action_count == actions.size()) {
      new_screen.set_active_entry(old_index);
      get_ui_session().render();
    }
  }
}

void shell_script_adapter::shutdown_process() {
  debugf("shell_script_binding: shutting down process\n");
  if (poll_timer_id != 0) {
    controller_api->cancel_timer(poll_timer_id);
    poll_timer_id = 0;
  }

  if (process.is_alive()) {
    debugf("shell_script_binding: sending SIGTERM to process\n");
    const auto res = process.terminate();
    if (res.has_value() && *res != 0)
      return controller_api->fatal_error(std::string("Failed to stop script: ") + strerror(*res));

    // Make sure it's dead
    auto lambda = [old_process = std::make_shared<subprocess::process>(std::move(process))] {
      if (old_process->is_alive()) {
        debugf("shell_script_binding: process did not exit after SIGTERM, sending SIGKILL\n");
        (void) old_process->kill();
      }
    };
    controller_api->schedule_timer(1000, false, std::move(lambda));
  }
}
