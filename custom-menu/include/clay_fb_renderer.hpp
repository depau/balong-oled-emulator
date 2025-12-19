#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "clay.hpp"
#include "debug.h"

#define bswap16(x) __builtin_bswap16(x)

inline constexpr float BW_LUMINANCE_THRESHOLD = 0.4f;

// ------------------------------------------------------------
// Font types
// ------------------------------------------------------------

struct Glyph {
  std::uint16_t width;
  std::uint16_t height;
  std::int16_t bearingX;
  std::int16_t bearingY;
  std::uint16_t advance;
  std::uint32_t bitmapOffset; // offset into Font::bitmap (bytes)
};

struct BitmapFont {
  const char *name;

  std::uint16_t size;
  std::int16_t ascent;
  std::int16_t descent; // negative
  std::int16_t lineGap;

  const Glyph *glyphs; // 128 entries, ASCII 0–127
  const std::uint8_t *bitmap; // 8bpp alpha

  [[nodiscard]] const Glyph &glyph(const std::uint32_t codepoint) const noexcept {
    const std::uint32_t idx = (codepoint < 128) ? codepoint : static_cast<std::uint32_t>('?');
    return glyphs[idx];
  }

  struct TextMetrics {
    std::int32_t width;
    std::int32_t height;
  };

  [[nodiscard]] TextMetrics measure(const std::string_view text) const noexcept {
    std::int32_t lineWidth = 0;
    std::int32_t maxWidth = 0;
    std::int32_t lines = 1;

    const std::int32_t lineHeight = ascent - descent + lineGap;

    for (const char ch : text) {
      if (ch == '\n') {
        maxWidth = std::max(maxWidth, lineWidth);
        lineWidth = 0;
        ++lines;
        continue;
      }
      const Glyph &g = glyph(static_cast<std::uint8_t>(ch));
      lineWidth += g.advance;
    }

    maxWidth = std::max(maxWidth, lineWidth);
    return TextMetrics{ maxWidth, lines * lineHeight };
  }
};

using font_registry_t = std::vector<const BitmapFont *>;

// ------------------------------------------------------------
// Geometry / clip helpers
// ------------------------------------------------------------

struct IntRect {
  int x;
  int y;
  int w;
  int h;
};

inline IntRect bbox_to_int(const Clay_BoundingBox &bb) {
  const int x0 = static_cast<int>(bb.x + 0.5f);
  const int y0 = static_cast<int>(bb.y + 0.5f);
  const int w = static_cast<int>(bb.width + 0.5f);
  const int h = static_cast<int>(bb.height + 0.5f);
  return { x0, y0, w, h };
}

inline bool intersect(const IntRect &a, const IntRect &b, IntRect &out) {
  const int x0 = std::max(a.x, b.x);
  const int y0 = std::max(a.y, b.y);
  const int x1 = std::min(a.x + a.w, b.x + b.w);
  const int y1 = std::min(a.y + a.h, b.y + b.h);
  if (x0 >= x1 || y0 >= y1)
    return false;
  out = { x0, y0, x1 - x0, y1 - y0 };
  return true;
}

// ------------------------------------------------------------
// Color helpers (Clay_Color -> BGR565)
// ------------------------------------------------------------

inline std::uint8_t clamp_channel(float c) {
  int v = static_cast<int>(c + 0.5f);
  v = std::clamp(v, 0, 255);
  return static_cast<std::uint8_t>(v);
}

// BGR565: 5 bits B, 6 bits G, 5 bits R
inline std::uint16_t pack_bgr565(const Clay_Color &color) {
  const std::uint8_t r = clamp_channel(color.r);
  const std::uint8_t g = clamp_channel(color.g);
  const std::uint8_t b = clamp_channel(color.b);

  const std::uint16_t b5 = static_cast<std::uint16_t>(b >> 3);
  const std::uint16_t g6 = static_cast<std::uint16_t>(g >> 2);
  const std::uint16_t r5 = static_cast<std::uint16_t>(r >> 3);

  return static_cast<std::uint16_t>((r5 << 11) | (g6 << 5) | (b5));
}

// ------------------------------------------------------------
// CRTP base renderer
// ------------------------------------------------------------

template<typename Derived>
class ClayRendererBase {
protected:
  std::uint16_t *fb;
  const font_registry_t &fonts;

  Derived &self() noexcept { return static_cast<Derived &>(*this); }
  const Derived &self() const noexcept { return static_cast<const Derived &>(*this); }

public:
  static constexpr int width = Derived::kWidth;
  static constexpr int height = Derived::kHeight;

  ClayRendererBase(std::uint16_t *fb, const font_registry_t &fonts) noexcept : fb(fb), fonts(fonts) {}

  [[nodiscard]] const BitmapFont *getFont(const std::uint16_t fontId, std::uint16_t /*size*/) const noexcept {
    if (fontId >= fonts.size()) {
      debugf("warning: requested fontId %u out of range (max %zu)\n", fontId, fonts.size() - 1);
      return nullptr;
    }
    return fonts[fontId];
  }

  void clearBgr565(const std::uint16_t colorBgr565) {
    static_assert(Derived::kWidth > 0 && Derived::kHeight > 0);
    const std::size_t n = static_cast<std::size_t>(Derived::kWidth) * static_cast<std::size_t>(Derived::kHeight);
    const std::uint16_t v = bswap16(colorBgr565);
    for (std::size_t i = 0; i < n; ++i) {
      fb[i] = v;
    }
  }

  void clearMono(const bool on) {
    static_assert(Derived::kWidth > 0 && Derived::kHeight > 0);
    const std::size_t bitCount = static_cast<std::size_t>(Derived::kWidth) * static_cast<std::size_t>(Derived::kHeight);
    const std::size_t wordCount = (bitCount + 15) / 16;
    const std::uint16_t v = bswap16(on ? 0xFFFFu : 0x0000u);
    for (std::size_t i = 0; i < wordCount; ++i) {
      fb[i] = v;
    }
  }

protected:
  void fillRect(const IntRect &r, const IntRect *clip, std::uint16_t colorBgr565, bool monoOn) {
    const IntRect bounds{ 0, 0, Derived::kWidth, Derived::kHeight };
    IntRect tmp;
    if (!intersect(bounds, r, tmp))
      return;
    if (clip) {
      if (!intersect(tmp, *clip, tmp))
        return;
    }

    for (int y = tmp.y; y < tmp.y + tmp.h; ++y) {
      for (int x = tmp.x; x < tmp.x + tmp.w; ++x) {
        self().putPixel(x, y, colorBgr565);
      }
    }
  }

  void strokeBorder(const IntRect &r, const IntRect *clip, const Clay_BorderRenderData &brd) {
    const std::uint16_t color = pack_bgr565(brd.color);
    const auto &w = brd.width;

    if (w.top > 0) {
      const IntRect top{ r.x, r.y, r.w, static_cast<int>(w.top) };
      fillRect(top, clip, color, true);
    }
    if (w.bottom > 0) {
      const IntRect bottom{ r.x, r.y + r.h - static_cast<int>(w.bottom), r.w, static_cast<int>(w.bottom) };
      fillRect(bottom, clip, color, true);
    }
    if (w.left > 0) {
      const IntRect left{ r.x, r.y, static_cast<int>(w.left), r.h };
      fillRect(left, clip, color, true);
    }
    if (w.right > 0) {
      const IntRect right{ r.x + r.w - static_cast<int>(w.right), r.y, static_cast<int>(w.right), r.h };
      fillRect(right, clip, color, true);
    }
  }

  void
  drawTextInternal(const IntRect &bb, const IntRect *clip, const BitmapFont &font, const Clay_TextRenderData &tdata) {
    const std::string_view text{ tdata.stringContents.chars, static_cast<std::size_t>(tdata.stringContents.length) };

    const int lineHeight = font.ascent - font.descent + font.lineGap;

    const int x = bb.x;
    const int y = bb.y + font.size; // baseline

    std::uint16_t color = pack_bgr565(tdata.textColor);

    const IntRect bounds{ 0, 0, Derived::kWidth, Derived::kHeight };
    IntRect clipped;
    const IntRect *effClip = nullptr;
    if (clip) {
      if (!intersect(bounds, *clip, clipped))
        return;
      effClip = &clipped;
    } else {
      effClip = &bounds;
    }

    int cursorX = x;
    int cursorY = y;

    for (const char ch : text) {
      if (ch == '\n') {
        cursorX = x;
        cursorY += lineHeight;
        continue;
      }

      const Glyph &g = font.glyph(static_cast<std::uint8_t>(ch));

      const int gx = cursorX + g.bearingX;
      const int gy = cursorY - g.bearingY;

      if (g.width > 0 && g.height > 0) {
        const std::uint8_t *bmpBase = font.bitmap + g.bitmapOffset;

        for (int yy = 0; yy < g.height; ++yy) {
          int py = gy + yy;
          if (py < effClip->y || py >= effClip->y + effClip->h)
            continue;
          const std::uint8_t *row = bmpBase + static_cast<std::size_t>(yy) * g.width; // g.width is rowBytes for 8bpp

          for (int xx = 0; xx < g.width; ++xx) {
            int px = gx + xx;
            if (px < effClip->x || px >= effClip->x + effClip->w)
              continue;

            std::uint8_t alpha = row[xx];
            if (alpha == 0)
              continue;

            self().putPixel(px, py, color, alpha);
          }
        }
      }

      cursorX += g.advance;
    }
  }

public:
  void render(const Clay_RenderCommandArray &cmdArray) {
    std::vector<IntRect> scissorStack;
    scissorStack.reserve(8);

    auto currentClip = [&]() -> const IntRect * {
      if (scissorStack.empty())
        return nullptr;
      return &scissorStack.back();
    };

    for (int32_t i = 0; i < cmdArray.length; ++i) {
      const Clay_RenderCommand &cmd = cmdArray.internalArray[i];
      const IntRect bb = bbox_to_int(cmd.boundingBox);

      switch (cmd.commandType) {
      case CLAY_RENDER_COMMAND_TYPE_NONE:
        debugf("render command: none\n");
        break;

      case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
        debugf("render command: rectangle[%d, %d, %d, %d] - color: #%02X%02X%02X%02X\n",
               bb.x,
               bb.y,
               bb.w,
               bb.h,
               static_cast<uint8_t>(cmd.renderData.rectangle.backgroundColor.r),
               static_cast<uint8_t>(cmd.renderData.rectangle.backgroundColor.g),
               static_cast<uint8_t>(cmd.renderData.rectangle.backgroundColor.b),
               static_cast<uint8_t>(cmd.renderData.rectangle.backgroundColor.a));
        std::uint16_t color = pack_bgr565(cmd.renderData.rectangle.backgroundColor);
        fillRect(bb, currentClip(), color, true);
        break;
      }

      case CLAY_RENDER_COMMAND_TYPE_BORDER: {
        debugf("render command: border[%d, %d, %d, %d] - color: #%02X%02X%02X%02X\n",
               bb.x,
               bb.y,
               bb.w,
               bb.h,
               static_cast<uint8_t>(cmd.renderData.border.color.r),
               static_cast<uint8_t>(cmd.renderData.border.color.g),
               static_cast<uint8_t>(cmd.renderData.border.color.b),
               static_cast<uint8_t>(cmd.renderData.border.color.a));
        strokeBorder(bb, currentClip(), cmd.renderData.border);
        break;
      }

      case CLAY_RENDER_COMMAND_TYPE_TEXT: {
        debugf("render command: text[%d, %d, %d, %d] - text: \"%.*s\", fontId: %d, fontSize: %d, color: "
               "#%02X%02X%02X%02X\n",
               bb.x,
               bb.y,
               bb.w,
               bb.h,
               static_cast<int>(cmd.renderData.text.stringContents.length),
               cmd.renderData.text.stringContents.chars,
               cmd.renderData.text.fontId,
               cmd.renderData.text.fontSize,
               static_cast<uint8_t>(cmd.renderData.text.textColor.r),
               static_cast<uint8_t>(cmd.renderData.text.textColor.g),
               static_cast<uint8_t>(cmd.renderData.text.textColor.b),
               static_cast<uint8_t>(cmd.renderData.text.textColor.a));
        const auto &t = cmd.renderData.text;
        const BitmapFont *font = getFont(t.fontId, t.fontSize);
        if (!font)
          break;
        drawTextInternal(bb, currentClip(), *font, t);
        break;
      }

      case CLAY_RENDER_COMMAND_TYPE_IMAGE:
        debugf("render command: image\n");
        // Not used in this example; can be implemented similarly
        break;

      case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
        debugf("render command: scissorStart[%d, %d, %d, %d]\n", bb.x, bb.y, bb.w, bb.h);
        IntRect newClip = bb;
        if (!scissorStack.empty()) {
          IntRect merged;
          if (!intersect(scissorStack.back(), newClip, merged)) {
            scissorStack.push_back(IntRect{ 0, 0, 0, 0 });
          } else {
            scissorStack.push_back(merged);
          }
        } else {
          scissorStack.push_back(newClip);
        }
        break;
      }

      case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
        debugf("render command: scissorEnd\n");
        if (!scissorStack.empty())
          scissorStack.pop_back();
        break;
      }

      case CLAY_RENDER_COMMAND_TYPE_CUSTOM:
        debugf("render command: custom\n");
        break;
      }
    }
  }
};

// ------------------------------------------------------------
// Concrete renderer: 128x128 BGR565
// ------------------------------------------------------------

class ClayBGR565Renderer : public ClayRendererBase<ClayBGR565Renderer> {
public:
  static constexpr int kWidth = 128;
  static constexpr int kHeight = 128;

  using Base = ClayRendererBase<ClayBGR565Renderer>;

  ClayBGR565Renderer(std::uint16_t *fb, const font_registry_t &fonts) noexcept : Base(fb, fonts) {}

  void putPixel(const int x, const int y, const std::uint16_t colorBgr565) const {
    if (x < 0 || y < 0 || x >= kWidth || y >= kHeight)
      return;
    const int idx = y * kWidth + x;
    fb[idx] = bswap16(colorBgr565);
  }

  void putPixel(const int x, const int y, const std::uint16_t fgColor, std::uint8_t alpha) const {
    if (alpha == 0)
      return;
    if (alpha == 255) {
      putPixel(x, y, fgColor);
      return;
    }

    const std::uint16_t bgColor = getPixel(x, y);

    // Unpack foreground color (BGR565 to 8-bit RGB)
    const std::uint8_t f_r_5 = (fgColor >> 11) & 0x1F;
    const std::uint8_t f_g_6 = (fgColor >> 5) & 0x3F;
    const std::uint8_t f_b_5 = (fgColor >> 0) & 0x1F;

    const std::uint8_t f_r = (f_r_5 << 3) | (f_r_5 >> 2); // 5-bit to 8-bit
    const std::uint8_t f_g = (f_g_6 << 2) | (f_g_6 >> 4); // 6-bit to 8-bit
    const std::uint8_t f_b = (f_b_5 << 3) | (f_b_5 >> 2); // 5-bit to 8-bit

    // Unpack background color (BGR565 to 8-bit RGB)
    const std::uint8_t b_r_5 = (bgColor >> 11) & 0x1F;
    const std::uint8_t b_g_6 = (bgColor >> 5) & 0x3F;
    const std::uint8_t b_b_5 = (bgColor >> 0) & 0x1F;

    const std::uint8_t b_r = (b_r_5 << 3) | (b_r_5 >> 2);
    const std::uint8_t b_g = (b_g_6 << 2) | (b_g_6 >> 4);
    const std::uint8_t b_b = (b_b_5 << 3) | (b_b_5 >> 2);

    // Blend channels (alpha is 0-255)
    const std::uint8_t out_r = static_cast<std::uint8_t>(((f_r * alpha) + (b_r * (255 - alpha))) / 255);
    const std::uint8_t out_g = static_cast<std::uint8_t>(((f_g * alpha) + (b_g * (255 - alpha))) / 255);
    const std::uint8_t out_b = static_cast<std::uint8_t>(((f_b * alpha) + (b_b * (255 - alpha))) / 255);

    // Pack back to BGR565
    const std::uint16_t outColor = ((out_r >> 3) << 11) | ((out_g >> 2) << 5) | (out_b >> 3);
    putPixel(x, y, outColor);
  }

  std::uint16_t getPixel(const int x, const int y) const {
    if (x < 0 || y < 0 || x >= kWidth || y >= kHeight)
      return 0; // Or some default transparent color
    const int idx = y * kWidth + x;
    return bswap16(fb[idx]);
  }

  void clear(const Clay_Color &c) { clearBgr565(pack_bgr565(c)); }
};

// ------------------------------------------------------------
// Concrete renderer: 128x64 1-bit mono in uint16_t[width*height/16]
// ------------------------------------------------------------

class ClayBW1Renderer : public ClayRendererBase<ClayBW1Renderer> {
public:
  static constexpr int kWidth = 128;
  static constexpr int kHeight = 64;

  using Base = ClayRendererBase<ClayBW1Renderer>;

  ClayBW1Renderer(std::uint16_t *fb, const font_registry_t &fonts) noexcept : Base(fb, fonts) {}

  bool getPixel(const int x, const int y) const {
    if (x < 0 || y < 0 || x >= kWidth || y >= kHeight)
      return false;

    const int pixelIndex = y * kWidth + x;
    const int wordIndex = pixelIndex / 16;
    const int bitIndex = 15 - (pixelIndex % 16);

    const std::uint16_t word = bswap16(fb[wordIndex]);
    return (word >> bitIndex) & 1u;
  }

  void putPixel(const int x, const int y, const std::uint16_t colorBgr565) const {
    if (x < 0 || y < 0 || x >= kWidth || y >= kHeight)
      return;

    // Convert color to luminance to decide on/off
    const std::uint8_t b5 = static_cast<std::uint8_t>((colorBgr565 >> 11) & 0x1F);
    const std::uint8_t g6 = static_cast<std::uint8_t>((colorBgr565 >> 5) & 0x3F);
    const std::uint8_t r5 = static_cast<std::uint8_t>(colorBgr565 & 0x1F);
    const std::uint8_t br = static_cast<std::uint8_t>(b5 << 3);
    const std::uint8_t gr = static_cast<std::uint8_t>(g6 << 2);
    const std::uint8_t rr = static_cast<std::uint8_t>(r5 << 3);
    const float lum = 0.2126f * rr + 0.7152f * gr + 0.0722f * br;
    const bool on = lum > 255.0f * BW_LUMINANCE_THRESHOLD;

    const int pixelIndex = y * kWidth + x;
    const int wordIndex = pixelIndex / 16;
    const int bitIndex = 15 - (pixelIndex % 16);

    std::uint16_t word = bswap16(fb[wordIndex]);
    const std::uint16_t mask = static_cast<std::uint16_t>(1u << bitIndex);
    if (on)
      word |= mask;
    else
      word &= static_cast<std::uint16_t>(~mask);
    fb[wordIndex] = bswap16(word);
  }

  void putPixel(const int x, const int y, const std::uint16_t fgColor, std::uint8_t alpha) const {
    if (alpha == 0)
      return;
    if (alpha == 255) {
      putPixel(x, y, fgColor);
      return;
    }

    bool bgIsOn = getPixel(x, y);

    // Unpack foreground color (BGR565 to 8-bit RGB)
    const std::uint8_t f_r_5 = (fgColor >> 11) & 0x1F;
    const std::uint8_t f_g_6 = (fgColor >> 5) & 0x3F;
    const std::uint8_t f_b_5 = (fgColor >> 0) & 0x1F;

    const std::uint8_t f_r = (f_r_5 << 3) | (f_r_5 >> 2);
    const std::uint8_t f_g = (f_g_6 << 2) | (f_g_6 >> 4);
    const std::uint8_t f_b = (f_b_5 << 3) | (f_b_5 >> 2);

    // The background is either black (0) or white (255)
    const std::uint8_t b_r = bgIsOn ? 255 : 0;
    const std::uint8_t b_g = bgIsOn ? 255 : 0;
    const std::uint8_t b_b = bgIsOn ? 255 : 0;

    // Blend channels
    const std::uint8_t out_r = static_cast<std::uint8_t>(((f_r * alpha) + (b_r * (255 - alpha))) / 255);
    const std::uint8_t out_g = static_cast<std::uint8_t>(((f_g * alpha) + (b_g * (255 - alpha))) / 255);
    const std::uint8_t out_b = static_cast<std::uint8_t>(((f_b * alpha) + (b_b * (255 - alpha))) / 255);

    // Pack back to BGR565 for luminance calculation
    const std::uint16_t outColor = ((out_r >> 3) << 11) | ((out_g >> 2) << 5) | (out_b >> 3);
    putPixel(x, y, outColor);
  }

  void clear(const bool on = false) { clearMono(on); }
};
