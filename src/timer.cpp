#include "timer.h"

uint32_t last_id = 0;

timer::timer(callback_t &&callback, const uint32_t interval) : callback(callback), interval(interval), id(++last_id) {
  reset();
}

timer::timer(callback_t &&callback, const uint32_t interval, const bool repeat) :
  callback(callback), interval(interval), repeat(repeat), id(++last_id) {
  reset();
}
