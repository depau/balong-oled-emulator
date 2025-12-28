#include <cassert>
#include <dlfcn.h>
#include <filesystem>
#include <mutex>

#include "display_controller.hpp"
#include "main_menu.hpp"
#include "so_app_loader.hpp"

namespace fs = std::filesystem;

// ReSharper disable once CppPassValueParameterByConstReference
// ReSharper disable once CppParameterMayBeConstPtrOrRef
static Clay_Dimensions clay_measure_text_impl(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData) {
  if (userData == nullptr) {
    std::cerr << "ClayMeasureText: display_controller is null\n";
    abort();
  }

  const display_controller *ctrl = static_cast<display_controller *>(userData);
  const font_registry_t &reg = ctrl->get_font_registry();
  if (config->fontId >= reg.size()) {
    std::cerr << "ClayMeasureText: invalid fontId " << config->fontId << '\n';
    abort();
  }

  const BitmapFont *font = reg[config->fontId];
  if (!font) {
    std::cerr << "ClayMeasureText: font is null " << config->fontId << '\n';
    abort();
  }

  const std::string_view sv{ text.chars, static_cast<std::string_view::size_type>(text.length) };
  const auto [width, height] = font->measure(sv);
  return Clay_Dimensions{ static_cast<float>(width), static_cast<float>(height) };
}

Clay_Dimensions display_controller::clay_measure_text(const Clay_StringSlice &text, Clay_TextElementConfig *config) {
  return clay_measure_text_impl(text, config, this);
}

display_controller::display_controller() {
  timer_create_ex = reinterpret_cast<uint32_t (*)(uint32_t, uint32_t, void (*)(void *), void *)>(
    dlsym(RTLD_DEFAULT, "osa_timer_create_ex"));
  timer_delete_ex = reinterpret_cast<uint32_t (*)(uint32_t)>(dlsym(RTLD_DEFAULT, "osa_timer_delete_ex"));
  get_msgQ_id = reinterpret_cast<uint32_t (*)(uint32_t)>(dlsym(RTLD_DEFAULT, "osa_get_msgQ_id"));
  msgQex_send = reinterpret_cast<uint32_t (*)(uint32_t, uint32_t *, uint32_t, uint32_t)>(
    dlsym(RTLD_DEFAULT, "osa_msgQex_send"));
  assert(timer_create_ex != nullptr && "Failed to locate osa_timer_create_ex");
  assert(timer_delete_ex != nullptr && "Failed to locate osa_timer_delete_ex");
  assert(get_msgQ_id != nullptr && "Failed to locate osa_get_msgQ_id");
  assert(msgQex_send != nullptr && "Failed to locate osa_msgQex_send");

  Clay_Initialize(arena, Clay_Dimensions{ 128, 128 }, errHandler);
  Clay_SetMeasureTextFunction(&clay_measure_text_impl, this);
  load_apps();
}

display_controller::~display_controller() {
  set_active_app(std::nullopt);
  app_loaders.clear();
  for (auto &[descriptor, userptr] : apps) {
    if (descriptor.on_teardown)
      descriptor.on_teardown(userptr, this);
  }
}

static std::optional<fs::path> deref_symlink(fs::path p) {
  for (int i = 0; i < 10; ++i) {
    if (!fs::is_symlink(p)) {
      return p;
    }
    std::error_code ec;
    fs::path target = fs::read_symlink(p, ec);
    if (ec) {
      std::cerr << "Failed to read symlink: " << p << " - " << ec.message() << std::endl;
      return std::nullopt;
    }
    fs::path next_path = p.parent_path() / target;
    p = next_path.lexically_normal();
  }
  std::cerr << "Too many levels of symlinks: " << p << std::endl;
  return std::nullopt;
}

static bool app_file_sort(const fs::path &a, const fs::path &b) {
  return a.filename().string() < b.filename().string();
}

void display_controller::load_apps() {
  assert(apps.empty() && "Apps have already been loaded");

  register_app_loader(".so", load_app_shared_object);

  void *main_menu_userptr = nullptr;
  apps.emplace_back(*register_main_menu_app(this, &main_menu_userptr), main_menu_userptr);

  std::vector<fs::path> files;
  for (const auto &path : app_lookup_paths) {
    if (!fs::is_directory(path)) {
      std::cerr << "App lookup path is not a directory: " << path << std::endl;
      continue;
    }
    for (const auto &entry : fs::directory_iterator(path)) {
      const auto realpath = deref_symlink(entry.path());
      if (!realpath.has_value() || !fs::is_regular_file(*realpath))
        continue;
      files.push_back(*realpath);
    }
  }

  std::sort(files.begin(), files.end(), app_file_sort);

  for (const auto &file_path : files) {
    const std::string ext = file_path.extension().string();
    const auto it = app_loaders.find(ext);
    if (it == app_loaders.end()) {
      std::cerr << "No app loader registered for extension: " << ext << std::endl;
      continue;
    }

    void *app_userptr = nullptr;
    auto &[loader, loader_userptr] = it->second;
    app_descriptor *descriptor = loader(loader_userptr, this, file_path.string().c_str(), &app_userptr);
    if (!descriptor) {
      std::cerr << "Failed to load app: " << file_path << std::endl;
      continue;
    }

    apps.emplace_back(*descriptor, app_userptr);
    std::cout << "Loaded app: " << descriptor->name << " from " << file_path << std::endl;
  }
}

void display_controller::set_active_app(const std::optional<size_t> app_index) {
  assert(!app_index.has_value() || app_index.value() < apps.size());
  if (app_index == active_app_index)
    return;
  if (active_app_index.has_value()) {
    if (auto &[descriptor, userptr] = apps[active_app_index.value()]; descriptor.on_leave) {
      descriptor.on_leave(userptr, this);
    }
  }
  active_app_index = app_index;
  if (active_app_index.has_value()) {
    auto &[descriptor, userptr] = apps[active_app_index.value()];
    assert(descriptor.on_enter != nullptr && "Active app must have on_enter callback");
    descriptor.on_enter(userptr, this);
  }
}

std::vector<std::string> display_controller::get_app_lookup_paths() {
  std::vector<std::string> app_lookup_paths;
  if (app_lookup_paths.empty()) {
    if (const char *env_paths = std::getenv("CUSTOM_MENU_APP_PATH"); env_paths != nullptr) {
      std::string paths_str(env_paths);
      size_t start = 0;
      size_t end = paths_str.find(':');
      while (end != std::string::npos) {
        app_lookup_paths.emplace_back(paths_str.substr(start, end - start));
        start = end + 1;
        end = paths_str.find(':', start);
      }
      app_lookup_paths.emplace_back(paths_str.substr(start));
    }

    app_lookup_paths.emplace_back("./apps/");

#ifdef APP_LOOKUP_PATH
    app_lookup_paths.emplace_back(APP_LOOKUP_PATH);
#endif
#ifdef INSTALL_PREFIX
    app_lookup_paths.emplace_back(std::string(INSTALL_PREFIX "/apps/"));
#endif
    app_lookup_paths.emplace_back("/online/scripts/");
  }
  return app_lookup_paths;
}

void display_controller::clay_render(const Clay_RenderCommandArray &cmds) {
  if (is_small_screen_mode) {
    ClayBW1Renderer renderer(secret_screen_buf, font_registry);
    renderer.clear(false);
    renderer.render(cmds);
  } else {
    ClayBGR565Renderer renderer(secret_screen_buf, font_registry);
    renderer.clear(Clay_Color{ 0, 0, 0, 255 });
    renderer.render(cmds);
  }
  lcd_refresh_screen(&secret_screen);
}

std::optional<uint16_t> display_controller::get_font(const std::string &fontName, int fontSize) const {
  for (uint16_t i = 0; i < font_registry.size(); ++i) {
    const auto &font = font_registry[i];
    if (font->name == fontName && font->size == fontSize)
      return i;
  }
  return std::nullopt;
}

// ReSharper disable once CppDFAConstantParameter
void display_controller::send_msg(const uint32_t msg_type) const {
  constexpr int DEFAULT_QUEUE_ID = 1001;

  const uint32_t msg_queue = get_msgQ_id(DEFAULT_QUEUE_ID);
  if (!msg_queue) {
    fprintf(stderr, "Failed to get message queue to close the menu\n");
    return;
  }
  uint32_t msg[2] = { msg_type, 0 };
  msgQex_send(msg_queue, msg, 2 * sizeof(uint32_t), 0);
}

void display_controller::set_active(const bool active) {
  if (active == is_active)
    return;

  if (!active) {
    set_active_app(std::nullopt);

    if (heartbeat_timer_helper.has_value()) {
      timer_delete_ex(heartbeat_timer_helper->get_timer_id());
      heartbeat_timer_helper.reset();
      std::scoped_lock lock(timer_mutex);
      active_timers.clear();
    }
  } else {
    set_active_app(0);

    std::scoped_lock lock(timer_mutex);
    heartbeat_timer_helper.emplace([this] { on_heartbeat_timer(); }, 0, true, 0);

    const uint32_t heartbeat_timer_id = timer_create_ex(FRAME_INTERVAL_MS,
                                                        true,
                                                        timer_helper::get_trampoline(),
                                                        heartbeat_timer_helper->get_userptr());
    heartbeat_timer_helper->set_timer_id(heartbeat_timer_id);
  }
  is_active = active;

  schedule_timer(
    [this] {
      if (is_active && active_app_index.has_value()) {
        auto &[descriptor, userptr] = apps[active_app_index.value()];
        if (descriptor.on_enter) {
          descriptor.on_enter(userptr, this);
        }
      }
    },
    16);

  send_msg(UI_MENU_EXIT);
}

void display_controller::switch_to_small_screen_mode() {
  if (is_small_screen_mode)
    return;

  is_small_screen_mode = true;
  secret_screen = { .sx = 0,
                    .height = height(),
                    .sy = 0,
                    .width = width(),
                    .buf_len = static_cast<uint16_t>(width() * height() / 8),
                    .buf = secret_screen_buf };

  Clay_SetLayoutDimensions(Clay_Dimensions{ 128, 64 });
}

void display_controller::on_keypress(const int button) {
  if (active_app_index.has_value()) {
    if (auto &[descriptor, userptr] = apps[active_app_index.value()]; descriptor.on_keypress) {
      descriptor.on_keypress(userptr, this, button);
    }
  }
}

void display_controller::goto_main_menu() {
  assert(!apps.empty() && "No apps loaded, cannot go to main menu");
  set_active_app(0);
}

void display_controller::fatal_error(const char *message, const bool unload_app) {
  debugf("display_controller::fatal_error: message=\"%s\", unload_app=%d\n", message, unload_app);
  app_error_message = message;
  const auto prev_active_index = active_app_index;
  goto_main_menu();
  if (unload_app && prev_active_index.has_value() && prev_active_index.value() != 0) {
    if (auto &[descriptor, userptr] = apps[prev_active_index.value()]; descriptor.on_teardown) {
      descriptor.on_teardown(userptr, this);
    }
    apps.erase(apps.begin() + static_cast<ptrdiff_t>(*prev_active_index));
  }
}

void display_controller::on_heartbeat_timer() {
  std::scoped_lock lock(timer_mutex);
  const auto now = std::chrono::steady_clock::now();

  for (auto it = active_timers.begin(); it != active_timers.end();) {
    if (it->is_marked_for_deletion()) {
      it = active_timers.erase(it);
      continue;
    }

    if (now >= it->get_expiration()) {
      try {
        (*it)();
      } catch (...) {
        std::cerr << "Exception in timer callback\n";
      }

      // Check if it was marked for deletion during callback
      if (it->is_marked_for_deletion()) {
        it = active_timers.erase(it);
        continue;
      }

      if (it->is_repeat()) {
        it->set_expiration(now + std::chrono::milliseconds(it->get_interval_ms()));
        ++it;
      } else {
        it = active_timers.erase(it);
      }
    } else {
      ++it;
    }
  }
}

uint32_t display_controller::schedule_timer(std::function<void()> &&callback, uint32_t interval_ms, bool repeat) {
  std::scoped_lock lock(timer_mutex);

  uint32_t id = next_timer_id++;
  active_timers.emplace_back(std::move(callback), id, repeat, interval_ms);

  timer_debugf("scheduled internal timer: id=%u, interval_ms=%u, repeat=%d\n", id, interval_ms, repeat);
  return id;
}

uint32_t display_controller::schedule_timer(void (*callback)(void *userptr),
                                            const uint32_t interval_ms,
                                            const bool repeat,
                                            void *userptr) {
  // Always use std::function timers so we can handle nested timer cancellations
  return schedule_timer([userptr, callback] { callback(userptr); }, interval_ms, repeat);
}

uint32_t display_controller::cancel_timer(const uint32_t timer_id) {
  std::scoped_lock lock(timer_mutex);

  for (auto &timer : active_timers) {
    if (timer.get_timer_id() == timer_id) {
      timer.mark_for_deletion();
      timer_debugf("canceled internal timer: id=%u\n", timer_id);
      return 0;
    }
  }

  return 1;
}
