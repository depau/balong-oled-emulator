#include <SDL_ttf.h>
#include <cstdint>
#include <fontconfig/fontconfig.h>
#include <iostream>
#include <string>
#include <vector>

#include "sdl_utils.h"

std::string find_sans_serif_font_path() {
  if (!FcInit()) {
    return {};
  }

  FcPattern *pat = FcPatternCreate();
  if (!pat) {
    FcFini();
    return {};
  }

  FcPatternAddString(pat, FC_FAMILY, reinterpret_cast<const FcChar8 *>("sans-serif"));
  FcConfigSubstitute(nullptr, pat, FcMatchPattern);
  FcDefaultSubstitute(pat);

  FcResult result;
  FcPattern *font = FcFontMatch(nullptr, pat, &result);
  std::string path;

  if (font && result == FcResultMatch) {
    FcChar8 *file = nullptr;
    if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch && file) {
      path = reinterpret_cast<char *>(file);
    }
    FcPatternDestroy(font);
  }

  FcPatternDestroy(pat);
  FcFini();

  return path;
}

void convert_bgr565_to_rgb888(const std::span<uint16_t> &bgr565_buf,
                              std::vector<uint32_t> &rgb888_buf,
                              const int width,
                              const int height) {
  const int pixel_count = width * height;
  for (int i = 0; i < pixel_count; ++i) {
    const uint16_t pixel = bgr565_buf[i];
    const auto r5 = static_cast<uint8_t>((pixel >> 11) & 0x1F);
    const auto g6 = static_cast<uint8_t>((pixel >> 5) & 0x3F);
    const auto b5 = static_cast<uint8_t>(pixel & 0x1F);

    const auto r = static_cast<uint8_t>((r5 * 255) / 31);
    const auto g = static_cast<uint8_t>((g6 * 255) / 63);
    const auto b = static_cast<uint8_t>((b5 * 255) / 31);

    rgb888_buf[i] = (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
  }
}

void fill_gradient(std::vector<uint16_t> &bgr565_buf, uint32_t w, uint32_t h) {
  for (uint32_t y = 0; y < h; ++y) {
    for (uint32_t x = 0; x < w; ++x) {
      const auto r5 = static_cast<uint8_t>((x * (pow2(5) - 1)) / (w - 1));
      const auto g6 = static_cast<uint8_t>((y * (pow2(6) - 1)) / (h - 1));
      constexpr auto b5 = static_cast<uint8_t>(pow2(5) - 1);

      const uint16_t rgb565 = (static_cast<uint16_t>(r5) << 11) | (static_cast<uint16_t>(g6) << 5) |
                              static_cast<uint16_t>(b5);

      bgr565_buf[y * w + x] = rgb565;
    }
  }
}

void draw_text(std::vector<uint16_t> &bgr565_buf,
               const uint32_t w,
               const uint32_t h,
               const std::string &text,
               TTF_Font *font) {
  if (!font) {
    return;
  }

  constexpr SDL_Color white = { 255, 255, 255, 255 };
  SDL_Surface *text_surface = TTF_RenderText_Blended_Wrapped(font, text.c_str(), white, w);
  if (!text_surface) {
    std::cerr << "Could not create text surface: " << TTF_GetError() << '\n';
    return;
  }

  // Find rightmost non-transparent pixel in text surface
  uint32_t text_width = 0;

  auto *base = static_cast<uint8_t *>(text_surface->pixels);
  const int pitch = text_surface->pitch; // bytes per row

  for (int y = 0; y < text_surface->h; ++y) {
    const auto *row_pixels = reinterpret_cast<uint32_t *>(base + y * pitch);
    for (int x = 0; x < text_surface->w; ++x) {
      const uint32_t pixel = row_pixels[x];

      uint8_t r, g, b, a;
      SDL_GetRGBA(pixel, text_surface->format, &r, &g, &b, &a);

      if (a > 0 && text_width < x)
        text_width = x;
    }
  }

  const int text_x = static_cast<int>(w - text_width) / 2;
  const int text_y = static_cast<int>(h - text_surface->h) / 2;

  if (SDL_MUSTLOCK(text_surface)) {
    if (SDL_LockSurface(text_surface) != 0) {
      std::cerr << "Could not lock text surface: " << SDL_GetError() << '\n';
      SDL_FreeSurface(text_surface);
      return;
    }
  }

  for (int y = 0; y < text_surface->h; ++y) {
    const auto *row_pixels = reinterpret_cast<uint32_t *>(base + y * pitch);
    for (int x = 0; x < text_surface->w; ++x) {
      const uint32_t pixel = row_pixels[x];

      uint8_t r, g, b, a;
      SDL_GetRGBA(pixel, text_surface->format, &r, &g, &b, &a);
      if (a == 0) {
        continue;
      }

      const int screen_x = text_x + x;
      const int screen_y = text_y + y;

      if (screen_x < 0 || screen_y < 0 || screen_x >= static_cast<int>(w) || screen_y >= static_cast<int>(h)) {
        continue;
      }

      const auto r5 = static_cast<uint16_t>(r >> 3);
      const auto g6 = static_cast<uint16_t>(g >> 2);
      const auto b5 = static_cast<uint16_t>(b >> 3);

      const auto rgb565 = static_cast<uint16_t>((r5 << 11) | (g6 << 5) | b5);

      bgr565_buf[screen_y * w + screen_x] = rgb565;
    }
  }

  if (SDL_MUSTLOCK(text_surface)) {
    SDL_UnlockSurface(text_surface);
  }
  SDL_FreeSurface(text_surface);
}

void dim_buffer(std::vector<uint32_t> &rgb888_buf, uint8_t brightness) {
  std::span bytes(reinterpret_cast<uint8_t *>(rgb888_buf.data()), rgb888_buf.size() * sizeof(uint32_t));
  for (auto &b : bytes) {
    b = b * brightness / 255;
  }
}
