#pragma once

#ifdef DEBUG
#define debugf(...) fprintf(stderr, __VA_ARGS__)
#else
#define debugf(...) \
  do {              \
  } while (0)
#endif

#ifdef DEBUG_TIMERS
#define timer_debugf(...) debugf(__VA_ARGS__)
#else
#define timer_debugf(...) \
  do {                    \
  } while (0)
#endif
