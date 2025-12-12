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

void convert_bw1bit_to_rgb888(const std::span<uint16_t> &bw1bit_buf, std::vector<uint32_t> &rgb888_buf) {
  rgb888_buf.clear();
  rgb888_buf.reserve(bw1bit_buf.size() * 8);
  for (auto pixel : bw1bit_buf) {
    pixel = __bswap_16(pixel);
    for (int i = 15; i >= 0; --i) {
      rgb888_buf.push_back(pixel & (1 << i) ? 0xFFFFFF : 0x000000);
    }
  }
}

void convert_bgr565_to_rgb888(const std::span<uint16_t> &bgr565_buf, std::vector<uint32_t> &rgb888_buf) {
  rgb888_buf.clear();
  rgb888_buf.reserve(bgr565_buf.size());
  for (uint16_t pixel : bgr565_buf) {
    pixel = __bswap_16(pixel);

    const auto b5 = static_cast<uint8_t>(pixel & 0x1F);
    pixel >>= 5;
    const auto g6 = static_cast<uint8_t>(pixel & 0x3F);
    pixel >>= 6;
    const auto r5 = static_cast<uint8_t>(pixel & 0x1F);

    const auto r = static_cast<uint8_t>((r5 * 255) / 31);
    const auto g = static_cast<uint8_t>((g6 * 255) / 63);
    const auto b = static_cast<uint8_t>((b5 * 255) / 31);

    rgb888_buf.push_back((static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b));
  }
}

void fill_gradient(std::vector<uint32_t> &rgb888_buf, uint32_t w, uint32_t h) {
  for (uint32_t y = 0; y < h; ++y) {
    for (uint32_t x = 0; x < w; ++x) {
      // Clamp the colors to simulate the limited color depth of the actual display
      const uint8_t r = static_cast<uint8_t>(255 * x / w) & 0b11111000;
      const uint8_t g = static_cast<uint8_t>(255 * y / h) & 0b11111100;
      const uint8_t b = static_cast<uint8_t>(255 * (w - x) / w) & 0b11111000;
      const auto rgb888 = static_cast<uint32_t>((r << 16) | (g << 8) | b);
      rgb888_buf[y * w + x] = rgb888;
    }
  }
}

void draw_text(std::vector<uint32_t> &rgb888_buf,
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

      const uint32_t rgb888 = static_cast<uint32_t>((r << 16) | (g << 8) | b);
      rgb888_buf[screen_y * w + screen_x] = rgb888;
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
