#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>

void tick();

void init_timer();

uint64_t get_millis_since_start();

struct microseconds {
  uint64_t val = 0;
};

struct milliseconds {
  uint64_t val = 0;
};

void busy_sleep(microseconds us);
void busy_sleep(milliseconds ms);

inline microseconds operator""_us(unsigned long long val) {
  return microseconds{val};
}

inline milliseconds operator""_ms(unsigned long long val) {
  return milliseconds{val};
}

#endif
