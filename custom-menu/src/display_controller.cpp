#include <cassert>
#include <filesystem>

#include "display_controller.hpp"
#include "main_menu.hpp"
#include "so_plugin_loader.hpp"

namespace fs = std::filesystem;

// ReSharper disable once CppPassValueParameterByConstReference
// ReSharper disable once CppParameterMayBeConstPtrOrRef
static Clay_Dimensions ClayMeasureText(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData) {
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

display_controller::display_controller() {
  Clay_Initialize(arena, Clay_Dimensions{ 128, 128 }, errHandler);
  Clay_SetMeasureTextFunction(&ClayMeasureText, this);
  load_plugins();
}

display_controller::~display_controller() {
  set_active_plugin(std::nullopt);
  plugin_loaders.clear();
  for (auto &[descriptor, userptr] : plugins) {
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

static bool plugin_file_sort(const fs::path &a, const fs::path &b) {
  return a.filename().string() < b.filename().string();
}

void display_controller::load_plugins() {
  assert(plugins.empty() && "Plugins have already been loaded");

  register_plugin_loader(".so", load_plugin_shared_object);

  void *main_menu_userptr = nullptr;
  plugins.emplace_back(*register_main_menu_plugin(this, &main_menu_userptr), main_menu_userptr);

  std::vector<fs::path> files;
  for (const auto &path : plugin_lookup_paths) {
    if (!fs::is_directory(path)) {
      std::cerr << "Plugin lookup path is not a directory: " << path << std::endl;
      continue;
    }
    for (const auto &entry : fs::directory_iterator(path)) {
      const auto realpath = deref_symlink(entry.path());
      if (!realpath.has_value() || !fs::is_regular_file(*realpath))
        continue;
      files.push_back(*realpath);
    }
  }

  std::ranges::sort(files, plugin_file_sort);

  for (const auto &file_path : files) {
    const std::string ext = file_path.extension().string();
    const auto it = plugin_loaders.find(ext);
    if (it == plugin_loaders.end()) {
      std::cerr << "No plugin loader registered for extension: " << ext << std::endl;
      continue;
    }

    void *plugin_userptr = nullptr;
    auto &[loader, loader_userptr] = it->second;
    plugin_descriptor *descriptor = loader(loader_userptr, this, file_path.string().c_str(), &plugin_userptr);
    if (!descriptor) {
      std::cerr << "Failed to load plugin: " << file_path << std::endl;
      continue;
    }

    plugins.emplace_back(*descriptor, plugin_userptr);
    std::cout << "Loaded plugin: " << descriptor->name << " from " << file_path << std::endl;
  }
}

void display_controller::set_active_plugin(std::optional<size_t> plugin_index) {
  assert(!plugin_index.has_value() || plugin_index.value() < plugins.size());
  if (plugin_index == active_plugin_index)
    return;
  if (active_plugin_index.has_value()) {
    if (auto &[descriptor, userptr] = plugins[active_plugin_index.value()]; descriptor.on_blur) {
      descriptor.on_blur(userptr, this);
    }
  }
  active_plugin_index = plugin_index;
  if (active_plugin_index.has_value()) {
    auto &[descriptor, userptr] = plugins[active_plugin_index.value()];
    assert(descriptor.on_focus != nullptr && "Active plugin must have on_focus callback");
    descriptor.on_focus(userptr, this);
  }
}

std::vector<std::string> display_controller::get_plugin_lookup_paths() {
  std::vector<std::string> plugin_lookup_paths;
  if (plugin_lookup_paths.empty()) {
    if (const char *env_paths = std::getenv("CUSTOM_MENU_PLUGIN_PATHS"); env_paths != nullptr) {
      std::string paths_str(env_paths);
      size_t start = 0;
      size_t end = paths_str.find(':');
      while (end != std::string::npos) {
        plugin_lookup_paths.emplace_back(paths_str.substr(start, end - start));
        start = end + 1;
        end = paths_str.find(':', start);
      }
      plugin_lookup_paths.emplace_back(paths_str.substr(start));
    }

    plugin_lookup_paths.emplace_back("./plugins/");

#ifdef PLUGIN_LOOKUP_PATH
    plugin_lookup_paths.emplace_back(PLUGIN_LOOKUP_PATH);
#endif
  }
  return plugin_lookup_paths;
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

void display_controller::set_active(const bool active) {
  if (active == is_active)
    return;

  if (!active) {
    set_active_plugin(std::nullopt);
  } else {
    set_active_plugin(0);
  }
  is_active = active;
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
  if (active_plugin_index.has_value()) {
    if (auto &[descriptor, userptr] = plugins[active_plugin_index.value()]; descriptor.on_keypress) {
      descriptor.on_keypress(userptr, this, button);
    }
  }
}

void display_controller::goto_main_menu() {
  assert(!plugins.empty() && "No plugins loaded, cannot go to main menu");
  set_active_plugin(0);
}

void display_controller::fatal_error(const char *message, bool unload_plugin) {
  plugin_error_message = message;
  auto prev_active_index = active_plugin_index;
  goto_main_menu();
  if (unload_plugin && prev_active_index.has_value() && prev_active_index.value() != 0) {
    if (auto &[descriptor, userptr] = plugins[prev_active_index.value()]; descriptor.on_teardown) {
      descriptor.on_teardown(userptr, this);
    }
    plugins.erase(plugins.begin() + static_cast<ptrdiff_t>(*prev_active_index));
  }
}
