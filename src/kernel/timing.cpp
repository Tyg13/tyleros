#include "timing.h"

#include "pit.h"
#include "scheduler.h"

const static auto ticks_per_second =
    (uint64_t)1193182; // TODO document where this came from (see PIT)
const static auto millis_per_second = 1000;
const static auto micros_per_milli = 1000;
const static auto ticks_per_micro =
    ticks_per_second / millis_per_second / micros_per_milli;

static uint64_t micros_since_start = 0;
static uint64_t millis_since_start = 0;

static unsigned int ticks_in_this_micro = 0;
static unsigned int micros_in_this_milli = 0;
void tick() {
  ++ticks_in_this_micro;
  if (ticks_in_this_micro > ticks_per_micro) {
    ++micros_since_start;
    ++micros_in_this_milli;
    if (micros_in_this_milli > micros_per_milli) {
      ++millis_since_start;
      micros_in_this_milli -= micros_per_milli;
    }
    ticks_in_this_micro -= ticks_per_micro;
  }
}

void init_timer() { init_pit(); }

uint64_t get_millis_since_start() { return millis_since_start; }
uint64_t get_micros_since_start() { return micros_since_start; }

void busy_sleep(microseconds us) {
  const auto begin = get_micros_since_start();
  while (get_micros_since_start() - begin < us.val)
    scheduler::yield();
}

void busy_sleep(milliseconds ms) {
  const auto begin = get_millis_since_start();
  while (get_millis_since_start() - begin < ms.val)
    scheduler::yield();
}
