#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>

void tick();

void init_timer();

uint64_t get_millis_since_start();

struct microseconds {
  uint64_t val = 0;
};
void busy_sleep(microseconds us);

inline microseconds operator""_us(unsigned long long val) {
  return microseconds{val};
}

#endif
