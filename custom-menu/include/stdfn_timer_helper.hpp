#pragma once

#include <functional>

template<typename T>
class stdfn_timer_helper {
  std::function<void()> callback;
  uint32_t timer_id = -1;
  uint32_t stdfn_timer_id = -1;
  bool repeat = false;
  T *timer_host = nullptr;

  static void trampoline(void *userptr) {
    auto *this_ptr = static_cast<const stdfn_timer_helper *>(userptr);
    (*this_ptr)();
  }

public:
  stdfn_timer_helper() = default;

  stdfn_timer_helper(std::function<void()> &&cb, T *timer_host, const uint32_t stdfn_timer_id, const bool repeat) :
    callback(std::move(cb)), stdfn_timer_id(stdfn_timer_id), repeat(repeat), timer_host(timer_host) {}

  stdfn_timer_helper &operator=(const stdfn_timer_helper &other) = delete;

  stdfn_timer_helper(const stdfn_timer_helper &other) = delete;

  stdfn_timer_helper &operator=(stdfn_timer_helper &&other) noexcept {
    if (this != &other) {
      callback = std::move(other.callback);
      timer_id = other.timer_id;
      stdfn_timer_id = other.stdfn_timer_id;
      repeat = other.repeat;
      timer_host = other.timer_host;
    }
    return *this;
  }

  stdfn_timer_helper(stdfn_timer_helper &&other) noexcept { *this = std::move(other); }

  void operator()() const {
    callback();
    timer_host->gc_stdfn_timer(stdfn_timer_id);
  }

  void set_timer_id(const uint32_t id) { timer_id = id; }

  [[nodiscard]] uint32_t get_timer_id() const { return timer_id; }

  [[nodiscard]] uint32_t get_stdfn_timer_id() const { return stdfn_timer_id; }

  [[nodiscard]] bool is_repeat() const { return repeat; }

  static void (*get_trampoline())(void *) { return &trampoline; }

  [[nodiscard]] void *get_userptr() const { return const_cast<stdfn_timer_helper *>(this); }
};
