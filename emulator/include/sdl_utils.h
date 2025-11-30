#pragma once

#include <span>
#include <string>
#include <vector>

constexpr int pow2(const int number) {
  return 1 << number;
}

std::string find_sans_serif_font_path();

void convert_bw1bit_to_rgb888(const std::span<uint16_t> &bw1bit_buf, std::vector<uint32_t> &rgb888_buf);

void convert_bgr565_to_rgb888(const std::span<uint16_t> &bgr565_buf, std::vector<uint32_t> &rgb888_buf);

void fill_gradient(std::vector<uint32_t> &rgb888_buf, uint32_t w, uint32_t h);

void draw_text(std::vector<uint32_t> &rgb888_buf, uint32_t w, uint32_t h, const std::string &text, TTF_Font *font);

void dim_buffer(std::vector<uint32_t> &rgb888_buf, uint8_t brightness);
