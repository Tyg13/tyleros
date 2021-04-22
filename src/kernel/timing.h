#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>

void tick();

void init_timer();

uint64_t get_millis_since_start();

#endif
