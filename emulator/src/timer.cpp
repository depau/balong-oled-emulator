#include <cassert>

#include "timer.h"

uint32_t last_id = 0;

timer::timer(callback_t &&callback, uint32_t interval, void *userptr, const bool repeat) :
  callback(callback), interval(interval), repeat(repeat), id(++last_id), userptr(userptr) {
  const auto now = deadline;
  reset();
  assert(deadline > now);
}
