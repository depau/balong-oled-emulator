#pragma once

#include <chrono>
#include <cstdint>
#include <functional>

class timer_helper {
  std::function<void()> callback;
  uint32_t timer_id = 0;
  bool repeat = false;
  uint32_t interval_ms = 0;
  std::chrono::steady_clock::time_point expiration{};
  bool marked_for_deletion = false;

  static void trampoline(void *userptr) {
    auto *this_ptr = static_cast<const timer_helper *>(userptr);
    (*this_ptr)();
  }

public:
  timer_helper() = default;

  timer_helper(std::function<void()> &&cb, const uint32_t timer_id, const bool repeat, const uint32_t interval_ms = 0) :
    callback(std::move(cb)), timer_id(timer_id), repeat(repeat), interval_ms(interval_ms) {
    if (interval_ms > 0) {
      expiration = std::chrono::steady_clock::now() + std::chrono::milliseconds(interval_ms);
    }
  }

  timer_helper(const timer_helper &) = delete;
  timer_helper &operator=(const timer_helper &) = delete;

  timer_helper(timer_helper &&other) noexcept { *this = std::move(other); }

  timer_helper &operator=(timer_helper &&other) noexcept {
    if (this != &other) {
      callback = std::move(other.callback);
      timer_id = other.timer_id;
      repeat = other.repeat;
      interval_ms = other.interval_ms;
      expiration = other.expiration;
      marked_for_deletion = other.marked_for_deletion;
    }
    return *this;
  }

  void operator()() const {
    if (callback) {
      callback();
    }
  }

  void set_timer_id(const uint32_t id) { timer_id = id; }
  [[nodiscard]] uint32_t get_timer_id() const { return timer_id; }

  [[nodiscard]] bool is_repeat() const { return repeat; }

  [[nodiscard]] uint32_t get_interval_ms() const { return interval_ms; }

  [[nodiscard]] std::chrono::steady_clock::time_point get_expiration() const { return expiration; }

  void set_expiration(const std::chrono::steady_clock::time_point &exp) { expiration = exp; }

  void mark_for_deletion() { marked_for_deletion = true; }

  [[nodiscard]] bool is_marked_for_deletion() const { return marked_for_deletion; }

  static void (*get_trampoline())(void *) { return &trampoline; }

  [[nodiscard]] void *get_userptr() const { return const_cast<timer_helper *>(this); }
};
