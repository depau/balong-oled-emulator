#pragma once
#include <chrono>
#include <functional>

using namespace std::chrono_literals;

class timer {
public:
  using callback_t = std::function<void()>;
  using time_point = std::chrono::steady_clock::time_point;

private:
  callback_t callback;
  time_point deadline = std::chrono::steady_clock::now();
  uint32_t interval = 0;
  bool repeat = true;
  uint32_t id = 0;

public:
  timer(callback_t &&callback, uint32_t interval);

  timer(callback_t &&callback, uint32_t interval, bool repeat);

  [[nodiscard]] uint32_t get_id() const { return id; }

  void run() const { callback(); }

  callback_t &get_callback() { return callback; }

  [[nodiscard]] bool is_expired() const { return std::chrono::steady_clock::now() >= deadline; }

  [[nodiscard]] bool should_repeat() const { return repeat; }

  void reset() { deadline = std::chrono::steady_clock::now() + 1ms * interval; }

  static bool compare_deadlines(const timer &lhs, const timer &rhs) { return lhs.deadline < rhs.deadline; }

  static bool compare_deadlines_reverse(const timer &lhs, const timer &rhs) { return lhs.deadline > rhs.deadline; }
};
