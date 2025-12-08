#pragma once

#ifdef DEBUG
#define debugf(...) fprintf(stderr, __VA_ARGS__)
#else
#define debugf(...) \
  do {              \
  } while (0)
#endif