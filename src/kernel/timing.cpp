#include "timing.h"

#include "pit.h"
#include "pic.h"

const static auto ticks_per_second = (uint64_t)1193182; // TODO document where this came from (see PIT)
const static auto millisecond_threshold = (ticks_per_second / 1000) + 1;

static uint64_t milliseconds_since_start = 0;
static unsigned int ticks_in_millisecond = 0;
void tick() {
   ++ticks_in_millisecond;
   if (ticks_in_millisecond >= millisecond_threshold) {
      ticks_in_millisecond -= millisecond_threshold;
      ++milliseconds_since_start;
   }
}

void init_timer() {
   init_pit();
}

uint64_t get_millis_since_start() {
   return milliseconds_since_start;
}
