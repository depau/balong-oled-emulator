#include "stdfn_timer_helper.hpp"

namespace timers_priv {
thread_local std::optional<uint32_t> current_timer_id = std::nullopt;
}

std::optional<uint32_t> get_running_timer_id() {
  return timers_priv::current_timer_id;
}
