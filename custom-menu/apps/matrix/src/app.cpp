#include <cstdlib>
#include <cstring>

#include "apps/app_api.h"
#include "apps/app_api.hpp"
#include "clay.hpp"
#include "ui/utils.hpp"

struct matrix_text_entry {
  int x;
  int y;
  char str[16 + 1];
  int speed; // pixels per tick
  Clay_TextElementConfig textConfig = {
    .textColor = { 0, 0, 0, 255 },
  };
};

class matrix_app {
  static constexpr size_t SEQ_LEN = 20;
  app_api_t controller_api = nullptr;
  std::array<matrix_text_entry, SEQ_LEN> matrix_seq{};
  std::optional<uint32_t> tick_timer_id = std::nullopt;

  void tick() {
    const size_t lcd_height = controller_api->get_screen_height();
    const size_t lcd_width = controller_api->get_screen_width();

    for (int i = 0; i < 20; i += 1) {
      matrix_seq[i].y = (matrix_seq[i].y + matrix_seq[i].speed) % 256;
      if (matrix_seq[i].y > lcd_height && matrix_seq[i].y < lcd_height + 15) {
        matrix_seq[i].x = std::rand() % lcd_width;
      }
    }

    Clay_BeginLayout();
    ROOT_ELEMENT(controller_api, CLAY_TOP_TO_BOTTOM) {
      for (matrix_text_entry &entry : matrix_seq) {

        for (const int y : { entry.y, entry.y - 256 }) {
          CLAY({
            .floating = (Clay_FloatingElementConfig) {
              .offset = {
                .x = static_cast<float>(entry.x),
                .y = static_cast<float>(y),
              },
              .attachTo = CLAY_ATTACH_TO_PARENT
            }
          }) {
            const Clay_String text = {
              .isStaticallyAllocated = false,
              .length = static_cast<int32_t>(std::strlen(entry.str)),
              .chars = entry.str,
            };
            CLAY_TEXT(text, &entry.textConfig);
          }
        }
      }
    }
    controller_api->clay_render(Clay_EndLayout());
  }

  void init() {
    for (int i = 0; i < SEQ_LEN; i += 1) {
      matrix_seq[i].x = std::rand() % 128;
      matrix_seq[i].y = std::rand() % 128;
      const int len = 8 + std::rand() % 8;
      for (int j = 0; j < 16; j += 1) {
        if (j % 2 == 1) {
          matrix_seq[i].str[j] = '\n';
        } else {
          if (j < len) {
            matrix_seq[i].str[j] = ' ' + std::rand() % 15;
          } else {
            matrix_seq[i].str[j] = ' ';
          }
        }
      }
      matrix_seq[i].textConfig.textColor.g = std::rand() % 256;
      matrix_seq[i].speed = 2 + std::rand() % 8;
    }
  }

public:
  matrix_app() { init(); }

  explicit matrix_app(const app_api_t controller_api) : controller_api(controller_api) { init(); }

  void on_enter(app_api_t) {
    if (tick_timer_id.has_value())
      return;

    tick();
    tick_timer_id = controller_api->schedule_timer(30, true, [this] { tick(); });
  }

  void on_leave(app_api_t) {
    if (tick_timer_id.has_value()) {
      controller_api->cancel_timer(tick_timer_id.value());
      tick_timer_id = std::nullopt;
    }
  }

  static void on_keypress(const app_api_t controller_api, int /*button*/) { controller_api->goto_main_menu(); }
};

DECLARE_CPP_APP("Matrix", matrix_app);
